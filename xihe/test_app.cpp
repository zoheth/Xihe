#include "test_app.h"

#include "backend/shader_module.h"
#include "platform/filesystem.h"
#include "rendering/subpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "rendering/render_context.h"
#include "rendering/rdg_passes/main_pass.h"
#include "rendering/rdg_passes/shadow_pass.h"
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

	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", render_context_->get_surface_extent());
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();

	rdg_builder_->add_pass<rendering::ShadowPass>("shadow_pass", *scene_, *camera);

	rdg_builder_->add_pass<rendering::MainPass>("main_pass", *scene_, *camera);

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
