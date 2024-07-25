#include "test_app.h"

#include "backend/shader_module.h"
#include "platform/filesystem.h"
#include "rendering/rdg_passes/composite_pass.h"
#include "rendering/rdg_passes/main_pass.h"
#include "rendering/rdg_passes/post_processing.h"
#include "rendering/rdg_passes/shadow_pass.h"
#include "rendering/render_context.h"
#include "rendering/subpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "rendering/subpasses/shadow_subpass.h"
#include "scene_graph/components/camera.h"

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

	{
		std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
		rendering::PassInfo                              pass_info{};

		for (uint32_t i = 0; i < kCascadeCount; ++i)
		{
			auto subpass = std::make_unique<rendering::ShadowSubpass>(*render_context_, backend::ShaderSource{"shadow/csm.vert"}, backend::ShaderSource{"shadow/csm.frag"}, *scene_, *p_cascade_script, i);
			subpass->set_depth_stencil_attachment(i);

			subpasses.push_back(std::move(subpass));

			pass_info.outputs.push_back({
			    rendering::RdgResourceType::kAttachment,
			    "shadow_map_" + std::to_string(i),
			    common::get_suitable_depth_format(get_device()->get_gpu().get_handle()),
			});
			pass_info.outputs.back().override_resolution = vk::Extent2D{2048, 2048};
		}

		rdg_builder_->add_raster_pass("shadow_pass", pass_info, std::move(subpasses));
	}

	{
		rendering::PassInfo pass_info{};
		pass_info.outputs = {
		    {rendering::RdgResourceType::kAttachment, "hdr image", vk::Format::eR16G16B16A16Sfloat},
		    {rendering::RdgResourceType::kAttachment, "depth image", common::get_suitable_depth_format(get_device()->get_gpu().get_handle())},
		    {rendering::RdgResourceType::kAttachment, "albedo image", vk::Format::eR8G8B8A8Unorm},
		    {rendering::RdgResourceType::kAttachment, "normal image", vk::Format::eA2B10G10R10UnormPack32},
		};

		auto geometry_vs   = backend::ShaderSource{"deferred/geometry.vert"};
		auto geometry_fs   = backend::ShaderSource{"deferred/geometry.frag"};
		auto scene_subpass = std::make_unique<rendering::GeometrySubpass>(*render_context_, std::move(geometry_vs), std::move(geometry_fs), *scene_, *camera);

		// Outputs are depth, albedo, and normal
		scene_subpass->set_output_attachments({1, 2, 3});

		auto lighting_vs      = backend::ShaderSource{"deferred/lighting.vert"};
		auto lighting_fs      = backend::ShaderSource{"deferred/lighting.frag"};
		auto lighting_subpass = std::make_unique<rendering::LightingSubpass>(*render_context_, std::move(lighting_vs), std::move(lighting_fs), *camera, *scene_, p_cascade_script);

		lighting_subpass->set_input_attachments({1, 2, 3});

		std::vector<std::unique_ptr<rendering::Subpass>> subpasses{std::move(scene_subpass), std::move(lighting_subpass)};
		rdg_builder_->add_raster_pass("main_pass", pass_info, std::move(subpasses));
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

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::TestApp>();
}
