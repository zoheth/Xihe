#include "shadow_pass.h"

#include "rendering/render_context.h"
#include "rendering/subpasses/shadow_subpass.h"
#include "scene_graph/scene.h"

namespace xihe::rendering
{
ShadowPass::ShadowPass(RenderContext &render_context, sg::Scene &scene, sg::Camera &camera) :
    render_context_{render_context}
{
	use_swapchain_image_ = false;

	auto vertex_shader   = backend::ShaderSource{"shadow/csm.vert"};
	auto fragment_shader = backend::ShaderSource{"shadow/csm.frag"};

	auto shadow_subpass = std::make_unique<rendering::ShadowSubpass>(render_context, std::move(vertex_shader), std::move(fragment_shader), scene, *dynamic_cast<sg::PerspectiveCamera *>(&camera), 0);

	shadow_subpass->set_output_attachments({0});

	std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
	subpasses.push_back(std::move(shadow_subpass));

	render_pipeline_ = std::make_unique<rendering::RenderPipeline>(std::move(subpasses));
}

std::unique_ptr<RenderTarget> ShadowPass::create_render_target()
{
	auto &device = render_context_.get_device();

	backend::ImageBuilder shadow_image_builder{2048, 2048, 1};
	shadow_image_builder.with_format(common::get_suitable_depth_format(device.get_gpu().get_handle()));
	shadow_image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	shadow_image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

	backend::Image shadow_map = shadow_image_builder.build(device);

	std::vector<backend::Image> images;
	images.push_back(std::move(shadow_map));

	return std::make_unique<RenderTarget>(std::move(images));
}

void ShadowPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const
{
	begin_draw(command_buffer, render_target);
	render_pipeline_->draw(command_buffer, render_target);
}

void ShadowPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	auto &shadowmap_views = render_target.get_views();

	common::ImageMemoryBarrier memory_barrier{};

	memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
	memory_barrier.new_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	memory_barrier.src_access_mask = vk::AccessFlagBits::eNone;
	memory_barrier.dst_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eTopOfPipe;
	memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;

	for (const auto &shadowmap : shadowmap_views)
	{
		command_buffer.image_memory_barrier(shadowmap, memory_barrier);
	}
}

void ShadowPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
}
}        // namespace xihe::rendering
