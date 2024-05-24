#include "gltf_loader.h"

#include "backend/buffer.h"
#include "backend/device.h"
#include "common/helpers.h"
#include "common/logging.h"
#include "common/timer.h"
#include "components/light.h"
#include "platform/filesystem.h"
#include "scene_graph/components/image.h"
#include "scene_graph/scene.h"

#include <future>

#include <ctpl_stl.h>

namespace xihe
{
namespace
{
void upload_image_to_gpu(backend::CommandBuffer &command_buffer, backend::Buffer &staging_buffer, sg::Image &image)
{
	// Clean up the image data, as they are copied in the staging buffer
	image.clear_data();

	{
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
		memory_barrier.new_layout      = vk::ImageLayout::eTransferDstOptimal;
		memory_barrier.dst_access_mask = vk::AccessFlagBits::eTransferWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eHost;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eTransfer;

		command_buffer.image_memory_barrier(image.get_vk_image_view(), memory_barrier);
	}

	// Create a buffer image copy for every mip level
	auto &mipmaps = image.get_mipmaps();

	std::vector<vk::BufferImageCopy> buffer_copy_regions(mipmaps.size());

	for (size_t i = 0; i < mipmaps.size(); ++i)
	{
		auto &mipmap      = mipmaps[i];
		auto &copy_region = buffer_copy_regions[i];

		copy_region.bufferOffset     = mipmap.offset;
		copy_region.imageSubresource = image.get_vk_image_view().get_subresource_layers();
		// Update miplevel
		copy_region.imageSubresource.mipLevel = mipmap.level;
		copy_region.imageExtent               = mipmap.extent;
	}

	command_buffer.copy_buffer_to_image(staging_buffer, image.get_vk_image(), buffer_copy_regions);

	{
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout = vk::ImageLayout::eTransferDstOptimal;
		memory_barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
		memory_barrier.src_access_mask = vk::AccessFlagBits::eTransferWrite;
		memory_barrier.dst_access_mask = vk::AccessFlagBits::eShaderRead;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eTransfer;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eFragmentShader;

		command_buffer.image_memory_barrier(image.get_vk_image_view(), memory_barrier);
	}
}
}        // namespace

std::unordered_map<std::string, bool> GltfLoader::supported_extensions_ = {
    {"KHR_lights_punctual", false}};

GltfLoader::GltfLoader(backend::Device &device) :
    device_{device}
{}

std::unique_ptr<sg::Scene> GltfLoader::read_scene_from_file(const std::string &file_name, int scene_index)
{
	std::string err;
	std::string warn;

	tinygltf::TinyGLTF loader;

	std::string gltf_file_path = fs::path::get(fs::path::Type::kAssets) + file_name;

	bool import_result = loader.LoadASCIIFromFile(&model_, &err, &warn, gltf_file_path);

	if (!import_result)
	{
		LOGE("Failed to load gltf file {}.", gltf_file_path.c_str());

		return nullptr;
	}

	if (!err.empty())
	{
		LOGE("Error loading gltf model: {}.", err.c_str());

		return nullptr;
	}

	if (!warn.empty())
	{
		LOGW("{}", warn.c_str());
	}

	size_t last_slash = file_name.find_last_of('/');

	model_path_ = file_name.substr(0, last_slash);

	if (last_slash == std::string::npos)
	{
		model_path_.clear();
	}

	return std::make_unique<sg::Scene>(load_scene(scene_index));
}

sg::Scene GltfLoader::load_scene(int scene_index)
{
	sg::Scene scene;

	scene.set_name("gltf_scene");

	// Check extensions
	for (auto &used_extension : model_.extensionsUsed)
	{
		auto it = supported_extensions_.find(used_extension);

		// Check if extension isn't supported by the GLTFLoader
		if (it == supported_extensions_.end())
		{
			// If extension is required then we shouldn't allow the scene to be loaded
			if (std::ranges::find(model_.extensionsRequired, used_extension) != model_.extensionsRequired.end())
			{
				throw std::runtime_error("Cannot load glTF file. Contains a required unsupported extension: " + used_extension);
			}
			else
			{
				// Otherwise, if extension isn't required (but is in the file) then print a warning to the user
				LOGW("glTF file contains an unsupported extension, unexpected results may occur: {}", used_extension);
			}
		}
		else
		{
			// Extension is supported, so enable it
			LOGI("glTF file contains extension: {}", used_extension);
			it->second = true;
		}
	}

	// Load lights
	std::vector<std::unique_ptr<sg::Light>> light_components = parse_khr_lights_punctual();

	scene.set_components(std::move(light_components));

	// Load samplers
	std::vector<std::unique_ptr<sg::Sampler>>
	    sampler_components(model_.samplers.size());

	for (size_t sampler_index = 0; sampler_index < model_.samplers.size(); sampler_index++)
	{
		auto sampler                      = parse_sampler(model_.samplers[sampler_index]);
		sampler_components[sampler_index] = std::move(sampler);
	}

	scene.set_components(std::move(sampler_components));

	Timer timer;
	timer.start();

	// Load images
	auto thread_count = std::thread::hardware_concurrency();
	thread_count      = thread_count == 0 ? 1 : thread_count;
	ctpl::thread_pool thread_pool(thread_count);

	auto image_count = to_u32(model_.images.size());

	std::vector<std::future<std::unique_ptr<sg::Image>>> image_component_futures;
	for (size_t image_index = 0; image_index < image_count; image_index++)
	{
		auto fut = thread_pool.push(
		    [this, image_index](size_t) {
			    auto image = parse_image(model_.images[image_index]);

			    LOGI("Loaded gltf image #{} ({})", image_index, model_.images[image_index].uri.c_str());

			    return image;
		    });

		image_component_futures.push_back(std::move(fut));
	}

	std::vector<std::unique_ptr<sg::Image>> image_components;

	// Upload images to GPU. We do this in batches of 64MB of data to avoid needing
	// double the amount of memory (all the images and all the corresponding buffers).
	// This helps keep memory footprint lower which is helpful on smaller devices.
	size_t image_index = 0;
	while (image_index < image_count)
	{
		std::vector<backend::Buffer> transient_buffers;

		auto &command_buffer = device_.request_command_buffer();

		command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, 0);

		size_t batch_size = 0;

		// Deal with 64MB of image data at a time to keep memory footprint low
		while (image_index < image_count && batch_size < 64 * 1024 * 1024)
		{
			// Wait for this image to complete loading, then stage for upload
			image_components.push_back(image_component_futures[image_index].get());

			auto &image = image_components[image_index];

			backend::Buffer stage_buffer = backend::Buffer::create_staging_buffer(device_, image->get_data());

			batch_size += image->get_data().size();

			upload_image_to_gpu(command_buffer, stage_buffer, *image);

			transient_buffers.push_back(std::move(stage_buffer));

			image_index++;
		}

		command_buffer.end();

		auto &queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

		queue.submit(command_buffer, device.request_fence());

		device_.get_fence_pool().wait();
		device_.get_fence_pool().reset();
		device_.get_command_pool().reset_pool();
		device_.wait_idle();

		// Remove the staging buffers for the batch we just processed
		transient_buffers.clear();
	}

	scene.set_components(std::move(image_components));

	auto elapsed_time = timer.stop();

	LOGI("Time spent loading images: {} seconds across {} threads.", vkb::to_string(elapsed_time), thread_count);

	// Load textures
	auto images                  = scene.get_components<sg::Image>();
	auto samplers                = scene.get_components<sg::Sampler>();
	auto default_sampler_linear  = create_default_sampler(TINYGLTF_TEXTURE_FILTER_LINEAR);
	auto default_sampler_nearest = create_default_sampler(TINYGLTF_TEXTURE_FILTER_NEAREST);
	bool used_nearest_sampler    = false;

	for (auto &gltf_texture : model_.textures)
	{
		auto texture = parse_texture(gltf_texture);

		assert(gltf_texture.source < images.size());
		texture->set_image(*images[gltf_texture.source]);

		if (gltf_texture.sampler >= 0 && gltf_texture.sampler < static_cast<int>(samplers.size()))
		{
			texture->set_sampler(*samplers[gltf_texture.sampler]);
		}
		else
		{
			if (gltf_texture.name.empty())
			{
				gltf_texture.name = images[gltf_texture.source]->get_name();
			}

			// Get the properties for the image format. We'll need to check whether a linear sampler is valid.
			const VkFormatProperties fmtProps = device_.get_gpu().get_format_properties(images[gltf_texture.source]->get_format());

			if (fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
			{
				texture->set_sampler(*default_sampler_linear);
			}
			else
			{
				texture->set_sampler(*default_sampler_nearest);
				used_nearest_sampler = true;
			}
		}

		scene.add_component(std::move(texture));
	}

	scene.add_component(std::move(default_sampler_linear));
	if (used_nearest_sampler)
		scene.add_component(std::move(default_sampler_nearest));

	// Load materials
	bool                            has_textures = scene.has_component<sg::Texture>();
	std::vector<vkb::sg::Texture *> textures;
	if (has_textures)
	{
		textures = scene.get_components<sg::Texture>();
	}

	for (auto &gltf_material : model.materials)
	{
		auto material = parse_material(gltf_material);

		for (auto &gltf_value : gltf_material.values)
		{
			if (gltf_value.first.find("Texture") != std::string::npos)
			{
				std::string tex_name = to_snake_case(gltf_value.first);

				assert(gltf_value.second.TextureIndex() < textures.size());
				vkb::sg::Texture *tex = textures[gltf_value.second.TextureIndex()];

				if (texture_needs_srgb_colorspace(gltf_value.first))
				{
					tex->get_image()->coerce_format_to_srgb();
				}

				material->textures[tex_name] = tex;
			}
		}

		for (auto &gltf_value : gltf_material.additionalValues)
		{
			if (gltf_value.first.find("Texture") != std::string::npos)
			{
				std::string tex_name = to_snake_case(gltf_value.first);

				assert(gltf_value.second.TextureIndex() < textures.size());
				vkb::sg::Texture *tex = textures[gltf_value.second.TextureIndex()];

				if (texture_needs_srgb_colorspace(gltf_value.first))
				{
					tex->get_image()->coerce_format_to_srgb();
				}

				material->textures[tex_name] = tex;
			}
		}

		scene.add_component(std::move(material));
	}

	auto default_material = create_default_material();

	// Load meshes
	auto materials = scene.get_components<sg::PBRMaterial>();

	for (auto &gltf_mesh : model.meshes)
	{
		auto mesh = parse_mesh(gltf_mesh);

		for (size_t i_primitive = 0; i_primitive < gltf_mesh.primitives.size(); i_primitive++)
		{
			const auto &gltf_primitive = gltf_mesh.primitives[i_primitive];

			auto submesh_name = fmt::format("'{}' mesh, primitive #{}", gltf_mesh.name, i_primitive);
			auto submesh      = std::make_unique<sg::SubMesh>(std::move(submesh_name));

			for (auto &attribute : gltf_primitive.attributes)
			{
				std::string attrib_name = attribute.first;
				std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::tolower);

				auto vertex_data = get_attribute_data(&model, attribute.second);

				if (attrib_name == "position")
				{
					assert(attribute.second < model.accessors.size());
					submesh->vertices_count = to_u32(model.accessors[attribute.second].count);
				}

				core::Buffer buffer{device,
				                    vertex_data.size(),
				                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				                    VMA_MEMORY_USAGE_CPU_TO_GPU};
				buffer.update(vertex_data);
				buffer.set_debug_name(fmt::format("'{}' mesh, primitive #{}: '{}' vertex buffer",
				                                  gltf_mesh.name, i_primitive, attrib_name));

				submesh->vertex_buffers.insert(std::make_pair(attrib_name, std::move(buffer)));

				sg::VertexAttribute attrib;
				attrib.format = get_attribute_format(&model, attribute.second);
				attrib.stride = to_u32(get_attribute_stride(&model, attribute.second));

				submesh->set_attribute(attrib_name, attrib);
			}

			if (gltf_primitive.indices >= 0)
			{
				submesh->vertex_indices = to_u32(get_attribute_size(&model, gltf_primitive.indices));

				auto format = get_attribute_format(&model, gltf_primitive.indices);

				auto index_data = get_attribute_data(&model, gltf_primitive.indices);

				switch (format)
				{
					case VK_FORMAT_R8_UINT:
						// Converts uint8 data into uint16 data, still represented by a uint8 vector
						index_data          = convert_underlying_data_stride(index_data, 1, 2);
						submesh->index_type = VK_INDEX_TYPE_UINT16;
						break;
					case VK_FORMAT_R16_UINT:
						submesh->index_type = VK_INDEX_TYPE_UINT16;
						break;
					case VK_FORMAT_R32_UINT:
						submesh->index_type = VK_INDEX_TYPE_UINT32;
						break;
					default:
						LOGE("gltf primitive has invalid format type");
						break;
				}

				submesh->index_buffer = std::make_unique<core::Buffer>(device,
				                                                       index_data.size(),
				                                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				                                                       VMA_MEMORY_USAGE_GPU_TO_CPU);
				submesh->index_buffer->set_debug_name(fmt::format("'{}' mesh, primitive #{}: index buffer",
				                                                  gltf_mesh.name, i_primitive));

				submesh->index_buffer->update(index_data);
			}
			else
			{
				submesh->vertices_count = to_u32(get_attribute_size(&model, gltf_primitive.attributes.at("POSITION")));
			}

			if (gltf_primitive.material < 0)
			{
				submesh->set_material(*default_material);
			}
			else
			{
				assert(gltf_primitive.material < materials.size());
				submesh->set_material(*materials[gltf_primitive.material]);
			}

			mesh->add_submesh(*submesh);

			scene.add_component(std::move(submesh));
		}

		scene.add_component(std::move(mesh));
	}

	device.get_fence_pool().wait();
	device.get_fence_pool().reset();
	device.get_command_pool().reset_pool();

	scene.add_component(std::move(default_material));

	// Load cameras
	for (auto &gltf_camera : model.cameras)
	{
		auto camera = parse_camera(gltf_camera);
		scene.add_component(std::move(camera));
	}

	// Load nodes
	auto meshes = scene.get_components<sg::Mesh>();

	std::vector<std::unique_ptr<sg::Node>> nodes;

	for (size_t node_index = 0; node_index < model.nodes.size(); ++node_index)
	{
		auto gltf_node = model.nodes[node_index];
		auto node      = parse_node(gltf_node, node_index);

		if (gltf_node.mesh >= 0)
		{
			assert(gltf_node.mesh < meshes.size());
			auto mesh = meshes[gltf_node.mesh];

			node->set_component(*mesh);

			mesh->add_node(*node);
		}

		if (gltf_node.camera >= 0)
		{
			auto cameras = scene.get_components<sg::Camera>();
			assert(gltf_node.camera < cameras.size());
			auto camera = cameras[gltf_node.camera];

			node->set_component(*camera);

			camera->set_node(*node);
		}

		if (auto extension = get_extension(gltf_node.extensions, KHR_LIGHTS_PUNCTUAL_EXTENSION))
		{
			auto lights      = scene.get_components<sg::Light>();
			int  light_index = extension->Get("light").Get<int>();
			assert(light_index < lights.size());
			auto light = lights[light_index];

			node->set_component(*light);

			light->set_node(*node);
		}

		nodes.push_back(std::move(node));
	}

	std::vector<std::unique_ptr<sg::Animation>> animations;

	// Load animations
	for (size_t animation_index = 0; animation_index < model.animations.size(); ++animation_index)
	{
		auto &gltf_animation = model.animations[animation_index];

		std::vector<sg::AnimationSampler> samplers;

		for (size_t sampler_index = 0; sampler_index < gltf_animation.samplers.size(); ++sampler_index)
		{
			auto gltf_sampler = gltf_animation.samplers[sampler_index];

			sg::AnimationSampler sampler;
			if (gltf_sampler.interpolation == "LINEAR")
			{
				sampler.type = sg::AnimationType::Linear;
			}
			else if (gltf_sampler.interpolation == "STEP")
			{
				sampler.type = sg::AnimationType::Step;
			}
			else if (gltf_sampler.interpolation == "CUBICSPLINE")
			{
				sampler.type = sg::AnimationType::CubicSpline;
			}
			else
			{
				LOGW("Gltf animation sampler #{} has unknown interpolation value", sampler_index);
			}

			auto input_accessor      = model.accessors[gltf_sampler.input];
			auto input_accessor_data = get_attribute_data(&model, gltf_sampler.input);

			const float *data = reinterpret_cast<const float *>(input_accessor_data.data());
			for (size_t i = 0; i < input_accessor.count; ++i)
			{
				sampler.inputs.push_back(data[i]);
			}

			auto output_accessor      = model.accessors[gltf_sampler.output];
			auto output_accessor_data = get_attribute_data(&model, gltf_sampler.output);

			switch (output_accessor.type)
			{
				case TINYGLTF_TYPE_VEC3:
				{
					const glm::vec3 *data = reinterpret_cast<const glm::vec3 *>(output_accessor_data.data());
					for (size_t i = 0; i < output_accessor.count; ++i)
					{
						sampler.outputs.push_back(glm::vec4(data[i], 0.0f));
					}
					break;
				}
				case TINYGLTF_TYPE_VEC4:
				{
					const glm::vec4 *data = reinterpret_cast<const glm::vec4 *>(output_accessor_data.data());
					for (size_t i = 0; i < output_accessor.count; ++i)
					{
						sampler.outputs.push_back(glm::vec4(data[i]));
					}
					break;
				}
				default:
				{
					LOGW("Gltf animation sampler #{} has unknown output data type", sampler_index);
					continue;
				}
			}

			samplers.push_back(sampler);
		}

		auto animation = std::make_unique<sg::Animation>(gltf_animation.name);

		for (size_t channel_index = 0; channel_index < gltf_animation.channels.size(); ++channel_index)
		{
			auto &gltf_channel = gltf_animation.channels[channel_index];

			sg::AnimationTarget target;
			if (gltf_channel.target_path == "translation")
			{
				target = sg::AnimationTarget::Translation;
			}
			else if (gltf_channel.target_path == "rotation")
			{
				target = sg::AnimationTarget::Rotation;
			}
			else if (gltf_channel.target_path == "scale")
			{
				target = sg::AnimationTarget::Scale;
			}
			else if (gltf_channel.target_path == "weights")
			{
				LOGW("Gltf animation channel #{} has unsupported target path: {}", channel_index, gltf_channel.target_path);
				continue;
			}
			else
			{
				LOGW("Gltf animation channel #{} has unknown target path", channel_index);
				continue;
			}

			float start_time{std::numeric_limits<float>::max()};
			float end_time{std::numeric_limits<float>::min()};

			for (auto input : samplers[gltf_channel.sampler].inputs)
			{
				if (input < start_time)
				{
					start_time = input;
				}
				if (input > end_time)
				{
					end_time = input;
				}
			}

			animation->update_times(start_time, end_time);

			animation->add_channel(*nodes[gltf_channel.target_node], target, samplers[gltf_channel.sampler]);
		}

		animations.push_back(std::move(animation));
	}

	scene.set_components(std::move(animations));

	// Load scenes
	std::queue<std::pair<sg::Node &, int>> traverse_nodes;

	tinygltf::Scene *gltf_scene{nullptr};

	if (scene_index >= 0 && scene_index < static_cast<int>(model.scenes.size()))
	{
		gltf_scene = &model.scenes[scene_index];
	}
	else if (model.defaultScene >= 0 && model.defaultScene < static_cast<int>(model.scenes.size()))
	{
		gltf_scene = &model.scenes[model.defaultScene];
	}
	else if (model.scenes.size() > 0)
	{
		gltf_scene = &model.scenes[0];
	}

	if (!gltf_scene)
	{
		throw std::runtime_error("Couldn't determine which scene to load!");
	}

	auto root_node = std::make_unique<sg::Node>(0, gltf_scene->name);

	for (auto node_index : gltf_scene->nodes)
	{
		traverse_nodes.push(std::make_pair(std::ref(*root_node), node_index));
	}

	while (!traverse_nodes.empty())
	{
		auto node_it = traverse_nodes.front();
		traverse_nodes.pop();

		assert(node_it.second < nodes.size());
		auto &current_node       = *nodes[node_it.second];
		auto &traverse_root_node = node_it.first;

		current_node.set_parent(traverse_root_node);
		traverse_root_node.add_child(current_node);

		for (auto child_node_index : model.nodes[node_it.second].children)
		{
			traverse_nodes.push(std::make_pair(std::ref(current_node), child_node_index));
		}
	}

	scene.set_root_node(*root_node);
	nodes.push_back(std::move(root_node));

	// Store nodes into the scene
	scene.set_nodes(std::move(nodes));

	// Create node for the default camera
	auto camera_node = std::make_unique<sg::Node>(-1, "default_camera");

	auto default_camera = create_default_camera();
	default_camera->set_node(*camera_node);
	camera_node->set_component(*default_camera);
	scene.add_component(std::move(default_camera));

	scene.get_root_node().add_child(*camera_node);
	scene.add_node(std::move(camera_node));

	if (!scene.has_component<vkb::sg::Light>())
	{
		// Add a default light if none are present
		vkb::add_directional_light(scene, glm::quat({glm::radians(-90.0f), 0.0f, glm::radians(30.0f)}));
	}

	return scene;
}

std::vector<std::unique_ptr<sg::Light>> GltfLoader::parse_khr_lights_punctual()
{
	if (is_extension_enabled(KHR_LIGHTS_PUNCTUAL_EXTENSION))
	{
		if (!model_.extensions.contains(KHR_LIGHTS_PUNCTUAL_EXTENSION) || !model_.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Has("lights"))
		{
			return {};
		}
		auto &khr_lights = model_.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Get("lights");

		std::vector<std::unique_ptr<sg::Light>> light_components(khr_lights.ArrayLen());

		for (size_t light_index = 0; light_index < khr_lights.ArrayLen(); ++light_index)
		{
			auto &khr_light = khr_lights.Get(static_cast<int>(light_index));

			// Spec states a light has to have a type to be valid
			if (!khr_light.Has("type"))
			{
				LOGE("KHR_lights_punctual extension: light {} doesn't have a type!", light_index);
				throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
			}

			auto light = std::make_unique<sg::Light>(khr_light.Get("name").Get<std::string>());

			sg::LightType       type;
			sg::LightProperties properties;

			// Get type
			auto &gltf_light_type = khr_light.Get("type").Get<std::string>();
			if (gltf_light_type == "point")
			{
				type = sg::LightType::kPoint;
			}
			else if (gltf_light_type == "spot")
			{
				type = sg::LightType::kSpot;
			}
			else if (gltf_light_type == "directional")
			{
				type = sg::LightType::kDirectional;
			}
			else
			{
				LOGE("KHR_lights_punctual extension: light type '{}' is invalid", gltf_light_type);
				throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
			}

			// Get properties
			if (khr_light.Has("color"))
			{
				properties.color = glm::vec3(
				    static_cast<float>(khr_light.Get("color").Get(0).Get<double>()),
				    static_cast<float>(khr_light.Get("color").Get(1).Get<double>()),
				    static_cast<float>(khr_light.Get("color").Get(2).Get<double>()));
			}

			if (khr_light.Has("intensity"))
			{
				properties.intensity = static_cast<float>(khr_light.Get("intensity").Get<double>());
			}

			if (type != sg::LightType::kDirectional)
			{
				properties.range = static_cast<float>(khr_light.Get("range").Get<double>());
				if (type != sg::LightType::kPoint)
				{
					if (!khr_light.Has("spot"))
					{
						LOGE("KHR_lights_punctual extension: spot light doesn't have a 'spot' property set", gltf_light_type);
						throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
					}

					properties.inner_cone_angle = static_cast<float>(khr_light.Get("spot").Get("innerConeAngle").Get<double>());

					if (khr_light.Get("spot").Has("outerConeAngle"))
					{
						properties.outer_cone_angle = static_cast<float>(khr_light.Get("spot").Get("outerConeAngle").Get<double>());
					}
					else
					{
						// Spec states default value is PI/4
						properties.outer_cone_angle = glm::pi<float>() / 4.0f;
					}
				}
			}
			else if (type == sg::LightType::kDirectional || type == sg::LightType::kSpot)
			{
				// The spec states that the light will inherit the transform of the node.
				// The light's direction is defined as the 3-vector (0.0, 0.0, -1.0) and
				// the rotation of the node orients the light accordingly.
				properties.direction = glm::vec3(0.0f, 0.0f, -1.0f);
			}

			light->set_light_type(type);
			light->set_properties(properties);

			light_components[light_index] = std::move(light);
		}

		return light_components;
	}
	else
	{
		return {};
	}
}

bool GltfLoader::is_extension_enabled(const std::string &requested_extension)
{
	auto it = supported_extensions_.find(requested_extension);
	if (it != supported_extensions_.end())
	{
		return it->second;
	}
	else
	{
		return false;
	}
}
}        // namespace xihe
