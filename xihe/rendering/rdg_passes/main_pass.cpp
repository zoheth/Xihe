#include "main_pass.h"

#include "rendering/render_context.h"
#include "rendering/subpasses/geometry_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/scene.h"

namespace xihe::rendering
{
MainPass::MainPass(const std::string &name, RenderContext &render_context, sg::Scene &scene, sg::Camera &camera, sg::CascadeScript *cascade_script_ptr) :
    RdgPass{name, render_context}
{
	// Geometry subpass
	auto geometry_vs   = backend::ShaderSource{"deferred/geometry.vert"};
	auto geometry_fs   = backend::ShaderSource{"deferred/geometry.frag"};
	auto scene_subpass = std::make_unique<rendering::GeometrySubpass>(render_context, std::move(geometry_vs), std::move(geometry_fs), scene, camera);

	// Outputs are depth, albedo, and normal
	scene_subpass->set_output_attachments({1, 2, 3});

	// Lighting subpass
	auto lighting_vs      = backend::ShaderSource{"deferred/lighting.vert"};
	auto lighting_fs      = backend::ShaderSource{"deferred/lighting.frag"};
	auto lighting_subpass = std::make_unique<rendering::LightingSubpass>(render_context, std::move(lighting_vs), std::move(lighting_fs), camera, scene, cascade_script_ptr);

	// Inputs are depth, albedo, and normal from the geometry subpass
	lighting_subpass->set_input_attachments({1, 2, 3});

	/*auto vertex_shader   = backend::ShaderSource{"tests/test.vert"};
	auto fragment_shader = backend::ShaderSource{"tests/test.frag"};

	auto test_subpass = std::make_unique<TestSubpass>(*render_context_, std::move(vertex_shader), std::move(fragment_shader));*/

	// subpasses.push_back(std::move(test_subpass));
	add_subpass(std::move(scene_subpass));
	add_subpass(std::move(lighting_subpass));

	load_store_ = std::vector<common::LoadStoreInfo>(4, {vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare});
	// Swapchain image
	load_store_[0].store_op = vk::AttachmentStoreOp::eStore;

	clear_value_    = std::vector<vk::ClearValue>(4, vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}});
	clear_value_[1] = vk::ClearDepthStencilValue{0.0f, 0};
}

std::unique_ptr<RenderTarget> MainPass::create_render_target(backend::Image &&swapchain_image) const
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

	return std::make_unique<RenderTarget>(std::move(images));
}

void MainPass::prepare(backend::CommandBuffer &command_buffer)
{
	get_render_target()->set_layout(0, vk::ImageLayout::eColorAttachmentOptimal);
	for (size_t i = 2; i < get_render_target()->get_views().size(); ++i)
	{
		get_render_target()->set_layout(static_cast<uint32_t>(i), vk::ImageLayout::eColorAttachmentOptimal);
	}
	RdgPass::prepare(command_buffer);
}

void MainPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents)
{
	auto &views = render_target.get_views();

	{
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

		command_buffer.image_memory_barrier(views[0], memory_barrier);
		render_target.set_layout(0, memory_barrier.new_layout);

		// Skip 1 as it is handled later as a depth-stencil attachment
		for (size_t i = 2; i < views.size(); ++i)
		{
			command_buffer.image_memory_barrier(views[i], memory_barrier);
			render_target.set_layout(static_cast<uint32_t>(i), memory_barrier.new_layout);
		}
	}

	{
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.new_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		memory_barrier.dst_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;

		command_buffer.image_memory_barrier(views[1], memory_barrier);
		render_target.set_layout(1, memory_barrier.new_layout);
	}

	/*auto &shadowmap_views = render_context_.get_active_frame().get_render_target("shadow_pass").get_views();
	common::ImageMemoryBarrier memory_barrier{};

	memory_barrier.old_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	memory_barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
	memory_barrier.src_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	memory_barrier.dst_access_mask = vk::AccessFlagBits::eShaderRead;
	memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eFragmentShader;

	for (const auto &shadowmap : shadowmap_views)
	{
		command_buffer.image_memory_barrier(shadowmap, memory_barrier);
	}*/

	RdgPass::begin_draw(command_buffer, render_target, contents);
}

void MainPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	RdgPass::end_draw(command_buffer, render_target);

	{
		auto                      &views = render_target.get_views();
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.new_layout      = vk::ImageLayout::ePresentSrcKHR;
		memory_barrier.src_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eBottomOfPipe;

		command_buffer.image_memory_barrier(views[0], memory_barrier);
		render_target.set_layout(0, memory_barrier.new_layout);
	}
}
}        // namespace xihe::rendering
