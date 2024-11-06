#include "test_app.h"

#include "backend/shader_module.h"
#include "backend/shader_compiler/glsl_compiler.h"
#include "platform/filesystem.h"

#include "rendering/post_processing.h"
#include "rendering/render_context.h"
#include "rendering/subpass.h"
#include "rendering/subpasses/composite_subpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "rendering/subpasses/meshlet_subpass.h"
#include "rendering/subpasses/shadow_subpass.h"
#include "scene_graph/components/camera.h"

xihe::TestApp::TestApp()
{
	// Adding device extensions
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	backend::GlslCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
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

	update_bindless_descriptor_sets();

	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", render_context_->get_surface_extent());
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();

	auto  cascade_script   = std::make_unique<sg::CascadeScript>("", *scene_, *dynamic_cast<sg::PerspectiveCamera *>(camera));
	auto *p_cascade_script = cascade_script.get();
	scene_->add_component(std::move(cascade_script));

	// shadow pass
	{
		shadowmap_sampler_ = rendering::get_shadowmap_sampler(*get_device());

		std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
		rendering::PassInfo                              pass_info{};

		for (uint32_t i = 0; i < kCascadeCount; ++i)
		{
			auto subpass = std::make_unique<rendering::ShadowSubpass>(*render_context_, backend::ShaderSource{"shadow/csm.vert"}, backend::ShaderSource{"shadow/csm.frag"}, *scene_, *p_cascade_script, i);
			subpass->set_depth_stencil_attachment(i);

			subpasses.push_back(std::move(subpass));

			pass_info.outputs.push_back({rendering::RdgResourceType::kAttachment,
			                             "shadow_map_" + std::to_string(i),
			                             common::get_suitable_depth_format(get_device()->get_gpu().get_handle()),
			                             vk::ImageUsageFlagBits::eSampled});

			pass_info.outputs.back().set_sampler(shadowmap_sampler_.get());

			pass_info.outputs.back().override_resolution = vk::Extent3D{2048, 2048, 1};
		}

		rdg_builder_->add_raster_pass("shadow_pass", std::move(pass_info), std::move(subpasses));
	}

	// main pass
	{
		rendering::PassInfo pass_info{};
		pass_info.inputs = {
		    {rendering::RdgResourceType::kAttachment, "shadow_map_0"},
		    {rendering::RdgResourceType::kAttachment, "shadow_map_1"},
		    {rendering::RdgResourceType::kAttachment, "shadow_map_2"}};

		vk::ImageUsageFlags rt_usage_flags = vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eTransientAttachment;

		pass_info.outputs = {
		    {rendering::RdgResourceType::kAttachment, "hdr", vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eSampled},
		    {rendering::RdgResourceType::kAttachment, "depth", common::get_suitable_depth_format(get_device()->get_gpu().get_handle()), rt_usage_flags},
		    {rendering::RdgResourceType::kAttachment, "albedo", vk::Format::eR8G8B8A8Unorm, rt_usage_flags},
		    {rendering::RdgResourceType::kAttachment, "normal", vk::Format::eA2B10G10R10UnormPack32, rt_usage_flags},
		};

		/*auto geometry_vs   = backend::ShaderSource{"deferred/geometry.vert"};
		auto geometry_fs   = backend::ShaderSource{"deferred/geometry.frag"};
		auto scene_subpass = std::make_unique<rendering::GeometrySubpass>(*render_context_, std::move(geometry_vs), std::move(geometry_fs), *scene_, *camera);*/
		auto scene_subpass = std::make_unique<rendering::MeshletSubpass>(*render_context_, std::nullopt, backend::ShaderSource{"deferred/geometry_mesh.mesh"}, backend::ShaderSource{"deferred/geometry_mesh.frag"}, *scene_, *camera);

		// Outputs are depth, albedo, and normal
		scene_subpass->set_output_attachments({1, 2, 3});

		auto lighting_vs      = backend::ShaderSource{"deferred/lighting.vert"};
		auto lighting_fs      = backend::ShaderSource{"deferred/lighting.frag"};
		auto lighting_subpass = std::make_unique<rendering::LightingSubpass>(*render_context_, std::move(lighting_vs), std::move(lighting_fs), *camera, *scene_, p_cascade_script);

		lighting_subpass->set_input_attachments({1, 2, 3});

		std::vector<std::unique_ptr<rendering::Subpass>> subpasses;
		subpasses.push_back(std::move(scene_subpass));
		subpasses.push_back(std::move(lighting_subpass));
		rdg_builder_->add_raster_pass("main_pass", std::move(pass_info), std::move(subpasses));
	}

	// blur pass
	{
		linear_sampler_ = rendering::get_linear_sampler(*get_device());
		rendering::PassInfo pass_info{};
		pass_info.inputs = {
		    {rendering::RdgResourceType::kAttachment, "hdr"}};

		for (uint32_t i = 0; i < 7; ++i)
		{
			pass_info.outputs.push_back({rendering::RdgResourceType::kAttachment, "blur_" + std::to_string(i), vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled});
			pass_info.outputs.back().modify_extent = [i](const vk::Extent3D &extent) { return vk::Extent3D{
				                                                                           std::max(1u, extent.width >> i),
				                                                                           std::max(1u, extent.height >> i),
				                                                                           std::max(1u, extent.depth >> i)}; };
			pass_info.outputs.back().set_sampler(linear_sampler_.get());
		}

		rdg_builder_->add_compute_pass("blur_pass", std::move(pass_info),
		                               {backend::ShaderSource("post_processing/threshold.comp"),
		                                backend::ShaderSource("post_processing/blur_down.comp"),
		                                backend::ShaderSource("post_processing/blur_up.comp")},
		                               rendering::render_blur);
	}

	// composite pass
	{
		rendering::PassInfo pass_info{};
		pass_info.inputs = {
		    {rendering::RdgResourceType::kAttachment, "hdr"},
		    {rendering::RdgResourceType::kReference, "blur_1"}};

		pass_info.outputs = {
		    {rendering::RdgResourceType::kSwapchain, "swapchain"}};

		auto                                             composite_vs = backend::ShaderSource{"post_processing/composite.vert"};
		auto                                             composite_fs = backend::ShaderSource{"post_processing/composite.frag"};
		auto                                             subpass      = std::make_unique<rendering::CompositeSubpass>(*render_context_, std::move(composite_vs), std::move(composite_fs));
		std::vector<std::unique_ptr<rendering::Subpass>> subpasses;
		subpasses.push_back(std::move(subpass));
		rdg_builder_->add_raster_pass("composite_pass", std::move(pass_info), std::move(subpasses));
	}

	/*rdg_builder_->add_pass<rendering::ShadowPass>("shadow_pass", rendering::RdgPassType::kRaster, *scene_, *p_cascade_script);

	rdg_builder_->add_pass<rendering::MainPass>("main_pass", rendering::RdgPassType::kRaster, *scene_, *camera, p_cascade_script);

	rdg_builder_->add_pass<rendering::BlurPass>("blur_pass", rendering::RdgPassType::kCompute);

	rdg_builder_->add_pass<rendering::CompositePass>("composite_pass", rendering::RdgPassType::kRaster);*/

	return true;
}

void xihe::TestApp::update(float delta_time)
{
	XiheApp::update(delta_time);
}

void xihe::TestApp::request_gpu_features(backend::PhysicalDevice &gpu)
{
	XiheApp::request_gpu_features(gpu);

	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, meshShader);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, meshShaderQueries);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, taskShader);
}

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::TestApp>();
}
