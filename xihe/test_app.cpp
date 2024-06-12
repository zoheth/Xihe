#include "test_app.h"

#include "backend/shader_module.h"
#include "platform/filesystem.h"
#include "rendering/subpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
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

std::unique_ptr<xihe::rendering::RenderTarget> xihe::TestApp::create_render_target(backend::Image &&swapchain_image)

void xihe::TestApp::draw_renderpass(backend::CommandBuffer &command_buffer, rendering::RenderTarget &render_target)
{
	set_viewport_and_scissor(command_buffer, render_target.get_extent());



	if (render_pipeline_)
	{
		render_pipeline_->draw(command_buffer, render_context_->get_active_frame().get_render_target());
	}

	command_buffer.get_handle().endRenderPass();
}

std::unique_ptr<xihe::rendering::RenderPipeline> xihe::TestApp::create_shadow_render_pipeline()
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

	create_shadow_render_pipeline();

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
