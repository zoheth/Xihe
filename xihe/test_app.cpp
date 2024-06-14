#include "test_app.h"

#include "backend/shader_module.h"
#include "platform/filesystem.h"
#include "rendering/subpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "rendering/render_context.h"
#include "rendering/rdg_passes/main_pass.h"
#include "rendering/subpasses/shadow_subpass.h"
#include "scene_graph/components/camera.h"

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


	// Shadow map
	backend::ImageBuilder shadow_image_builder{2048,2048,1};
	shadow_image_builder.with_format(common::get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle()));
	shadow_image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	shadow_image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

	backend::Image shadow_map = shadow_image_builder.build(device);
	

	std::vector<backend::Image> images;

	// Attachment 0
	images.push_back(std::move(swapchain_image));

	// Attachment 1
	images.push_back(std::move(depth_image));

	// Attachment 2
	images.push_back(std::move(albedo_image));

	// Attachment 3
	images.push_back(std::move(normal_image));

	// Attachment 4
	images.push_back(std::move(shadow_map));

	return std::make_unique<rendering::RenderTarget>(std::move(images));
}

void xihe::TestApp::create_shadow_render_pipeline()
{
	auto vertex_shader   = backend::ShaderSource{"shadow/csm.vert"};
	auto fragment_shader = backend::ShaderSource{"shadow.frag"};

	auto shadow_subpass = std::make_unique<rendering::ShadowSubpass>(*render_context_, std::move(vertex_shader), std::move(fragment_shader), *scene_, *dynamic_cast<sg::PerspectiveCamera *>(camera_), 0);

	shadow_subpass->set_output_attachments({4});

	std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
	subpasses.push_back(std::move(shadow_subpass));

	shadow_render_pipeline_ = std::make_unique<rendering::RenderPipeline>(std::move(subpasses));
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
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();

	get_render_context().add_pass<rendering::MainPass>("main_pass", *scene_);

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
