#include "composite_pass.h"

#include "rendering/render_context.h"
#include "rendering/subpasses/composite_subpass.h"

namespace xihe::rendering
{
CompositePass::CompositePass(std::string name, RenderContext &render_context, RdgPassType pass_type) :
    RdgPass(std::move(name), render_context, pass_type)
{
	auto subpass = std::make_unique<CompositeSubpass>(render_context, backend::ShaderSource("post_processing/composite.vert"),
	                                                  backend::ShaderSource("post_processing/composite.frag"));

	add_subpass(std::move(subpass));

	auto sampler_info         = vk::SamplerCreateInfo{};
	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	sampler_ = std::make_unique<backend::Sampler>(get_device(), sampler_info);
}

void CompositePass::prepare(backend::CommandBuffer &command_buffer)
{
	CompositeSubpass &composite_subpass = dynamic_cast<CompositeSubpass &>(*subpasses_[0]);
	auto &main_pass_views = render_context_.get_active_frame().get_render_target("main_pass").get_views();
	auto &post_processing_views = render_context_.get_active_frame().get_render_target("post_processing").get_views();
	composite_subpass.set_texture(&main_pass_views[0], &post_processing_views[1], sampler_.get());
}

std::unique_ptr<RenderTarget> CompositePass::create_render_target(backend::Image &&swapchain_image) const
{
	std::vector<backend::Image> images;

	// Attachment 0
	images.push_back(std::move(swapchain_image));

	return std::make_unique<RenderTarget>(std::move(images));
}

void CompositePass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
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
