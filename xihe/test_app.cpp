#include "test_app.h"

#include "backend/shader_module.h"
#include "platform/filesystem.h"
#include "rendering/subpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "scene_graph/components/camera.h"

xihe::TestSubpass::TestSubpass(rendering::RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader) :
    rendering::Subpass{render_context, (std::move(vertex_shader)), (std::move(fragment_shader))}
{}

std::unique_ptr<xihe::rendering::RenderTarget> xihe::TestApp::create_render_target(backend::Image &&swapchain_image)
{
	auto &device = swapchain_image.get_device();
	auto &extent = swapchain_image.get_extent();

	backend::ImageBuilder image_builder{extent};

	vk::ImageUsageFlags rt_usage_flags = vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eTransientAttachment;

	image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

	image_builder.with_format(common::get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle()));
	image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
	    .with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | rt_usage_flags);
	backend::Image depth_image = image_builder.build(device);

	image_builder.with_format(vk::Format::eR8G8B8A8Unorm);
	image_builder.with_usage(vk::ImageUsageFlagBits::eColorAttachment | rt_usage_flags);
	backend::Image albedo_image = image_builder.build(device);

	image_builder.with_format(vk::Format::eA2B10G10R10UnormPack32);
	image_builder.with_usage(vk::ImageUsageFlagBits::eColorAttachment | rt_usage_flags);
	backend::Image normal_image = image_builder.build(device);

	std::vector<backend::Image> images;

	// Attachment 0
	images.push_back(std::move(swapchain_image));

	// Attachment 1
	images.push_back(std::move(depth_image));

	// Attachment 2
	images.push_back(std::move(albedo_image));

	// Attachment 3
	images.push_back(std::move(normal_image));

	return std::make_unique<rendering::RenderTarget>(std::move(images));
}

void xihe::TestSubpass::prepare()
{
	auto &resource_cache = render_context_.get_device().get_resource_cache();

	vertex_shader_   = std::make_unique<backend::ShaderSource>("tests/test.vert");
	fragment_shader_ = std::make_unique<backend::ShaderSource>("tests/test.frag");

	resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, *vertex_shader_);
	resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, *fragment_shader_);

	vertex_input_state_.bindings.emplace_back(0, to_u32(sizeof(Vertex)), vk::VertexInputRate::eVertex);

	vertex_input_state_.attributes.emplace_back(0, 0, vk::Format::eR32G32B32Sfloat, to_u32(offsetof(Vertex, position)));

	backend::BufferBuilder buffer_builder{sizeof(Vertex) * 3};
	buffer_builder
	    .with_usage(vk::BufferUsageFlagBits::eVertexBuffer)
	    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

	vertex_buffer_ = buffer_builder.build_unique(render_context_.get_device());

	std::vector<Vertex> vertices = {
	    {{0.0f, -0.5f, 0.0f}},
	    {{0.5f, 0.5f, 0.0f}},
	    {{-0.5f, 0.5f, 0.0f}},
	};

	vertex_buffer_->update(vertices.data(), vertices.size() * sizeof(Vertex));
}

void xihe::TestSubpass::draw(backend::CommandBuffer &command_buffer)
{
	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, *vertex_shader_);
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, *fragment_shader_);

	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	RasterizationState rasterization_state{};
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	command_buffer.set_vertex_input_state(vertex_input_state_);

	std::vector<std::reference_wrapper<const backend::Buffer>> buffers;
	buffers.emplace_back(std::ref(*vertex_buffer_));
	command_buffer.bind_vertex_buffers(0, buffers, {0});

	command_buffer.draw(3, 1, 0, 0);
}

bool xihe::TestApp::prepare(Window *window)
{
	if (!XiheApp::prepare(window))
	{
		return false;
	}

	get_render_context().update_swapchain(
	    {vk::ImageUsageFlagBits::eColorAttachment,
	     vk::ImageUsageFlagBits::eInputAttachment});

	load_scene("scenes/sponza/Sponza01.gltf");
	// load_scene("scenes/cube.gltf");
	assert(scene_ && "Scene not loaded");
	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", render_context_->get_surface_extent());
	camera_           = &camera_node.get_component<xihe::sg::Camera>();

	// Geometry subpass
	auto geometry_vs   = backend::ShaderSource{"deferred/geometry.vert"};
	auto geometry_fs   = backend::ShaderSource{"deferred/geometry.frag"};
	auto scene_subpass = std::make_unique<rendering::GeometrySubpass>(*render_context_, std::move(geometry_vs), std::move(geometry_fs), *scene_, *camera_);

	// Outputs are depth, albedo, and normal
	scene_subpass->set_output_attachments({1, 2, 3});

	// Lighting subpass
	auto lighting_vs      = backend::ShaderSource{"deferred/lighting.vert"};
	auto lighting_fs      = backend::ShaderSource{"deferred/lighting.frag"};
	auto lighting_subpass = std::make_unique<rendering::LightingSubpass>(*render_context_, std::move(lighting_vs), std::move(lighting_fs), *camera_, *scene_);

	// Inputs are depth, albedo, and normal from the geometry subpass
	lighting_subpass->set_input_attachments({1, 2, 3});

	/*auto vertex_shader   = backend::ShaderSource{"tests/test.vert"};
	auto fragment_shader = backend::ShaderSource{"tests/test.frag"};

	auto test_subpass = std::make_unique<TestSubpass>(*render_context_, std::move(vertex_shader), std::move(fragment_shader));*/

	std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
	// subpasses.push_back(std::move(test_subpass));
	subpasses.push_back(std::move(scene_subpass));
	subpasses.push_back(std::move(lighting_subpass));

	render_pipeline_ = std::make_unique<rendering::RenderPipeline>(std::move(subpasses));

	std::vector<common::LoadStoreInfo> load_store{4};

	// Swapchain image
	load_store[0].load_op  = vk::AttachmentLoadOp::eClear;
	load_store[0].store_op = vk::AttachmentStoreOp::eStore;

	// Depth image
	load_store[1].load_op  = vk::AttachmentLoadOp::eClear;
	load_store[1].store_op = vk::AttachmentStoreOp::eDontCare;

	// Albedo image
	load_store[2].load_op  = vk::AttachmentLoadOp::eClear;
	load_store[2].store_op = vk::AttachmentStoreOp::eDontCare;

	// Normal
	load_store[3].load_op  = vk::AttachmentLoadOp::eClear;
	load_store[3].store_op = vk::AttachmentStoreOp::eDontCare;

	render_pipeline_->set_load_store(load_store);

	std::vector<vk::ClearValue> clear_value{4};
	clear_value[0].color        = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_value[1].depthStencil = vk::ClearDepthStencilValue{0.0f, 0};
	clear_value[2].color        = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_value[3].color        = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};

	render_pipeline_->set_clear_value(clear_value);

	return true;
}

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::TestApp>();
}
