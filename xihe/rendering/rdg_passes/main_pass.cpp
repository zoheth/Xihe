#include "main_pass.h"

#include "rendering/render_context.h"
#include "rendering/subpasses/geometry_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/scene.h"

namespace xihe::rendering
{
MainPass::MainPass(const std::string &name, RenderContext &render_context, sg::Scene &scene, sg::Camera &camera) :
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
	auto lighting_subpass = std::make_unique<rendering::LightingSubpass>(render_context, std::move(lighting_vs), std::move(lighting_fs), camera, scene);

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
}

std::unique_ptr<RenderTarget> MainPass::create_render_target(backend::Image &&swapchain_image)
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

void MainPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const
{
	begin_draw(command_buffer, render_target);

	render_pipeline_->draw(command_buffer, render_target);

	end_draw(command_buffer, render_target);
}

void MainPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	auto &views = render_target.get_views();

	{
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.dst_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;

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
		memory_barrier.dst_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eTopOfPipe;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;

		command_buffer.image_memory_barrier(views[1], memory_barrier);
		render_target.set_layout(1, memory_barrier.new_layout);
	}
}

void MainPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	{
		auto                      &views = render_target.get_views();
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.new_layout      = vk::ImageLayout::ePresentSrcKHR;
		memory_barrier.src_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eBottomOfPipe;

		command_buffer.image_memory_barrier(views[0], memory_barrier);
		render_target.set_layout(0, memory_barrier.new_layout);
	}
}
}        // namespace xihe::rendering
