#include "post_processing.h"

#include "rendering/render_context.h"

namespace 
{
vk::Extent3D downsample_extent(const vk::Extent3D &extent, uint32_t level)
{
	return {
	    std::max(1u, extent.width >> level),
	    std::max(1u, extent.height >> level),
	    std::max(1u, extent.depth >> level)};
}

}

namespace xihe::rendering
{
BlurPass::BlurPass(std::string name, RenderContext &render_context, RdgPassType pass_type) :
    RdgPass(std::move(name), render_context, pass_type)
{
	auto &threshold_module     = get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eCompute,
	                                                                                     backend::ShaderSource("post_processing/threshold.comp"));
	auto &blur_up_module       = get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eCompute,
	                                                                                     backend::ShaderSource("post_processing/blur_up.comp"));
	auto &blur_down_module     = get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eCompute, backend::ShaderSource("post_processing/blur_down.comp"));
	threshold_pipeline_layout_ = &get_device().get_resource_cache().request_pipeline_layout({&threshold_module});
	blur_up_pipeline_layout_   = &get_device().get_resource_cache().request_pipeline_layout({&blur_up_module});
	blur_down_pipeline_layout_ = &get_device().get_resource_cache().request_pipeline_layout({&blur_down_module});

	auto sampler_info         = vk::SamplerCreateInfo{};
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	linear_sampler_ = std::make_unique<backend::Sampler>(get_device(), sampler_info);
}

std::unique_ptr<RenderTarget> BlurPass::create_render_target(backend::Image &&swapchain_image) const
{
	std::vector<backend::Image> blur_chain;
	auto &extent = swapchain_image.get_extent();
	for (uint32_t level = 1; level < 7; ++level)
	{
		backend::ImageBuilder image_builder{downsample_extent(extent, level)};
		image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
		.with_format(vk::Format::eR16G16B16A16Sfloat)
		.with_usage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled);
		blur_chain.push_back(image_builder.build(get_device()));
	}

	return std::make_unique<RenderTarget>(std::move(blur_chain));
}

void BlurPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers)
{

	// todo
	{
		common::ImageMemoryBarrier memory_barrier;
		memory_barrier.old_layout = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.new_layout = vk::ImageLayout::eReadOnlyOptimal;
		memory_barrier.src_access_mask = vk::AccessFlagBits2::eNone;
		memory_barrier.dst_access_mask = vk::AccessFlagBits2::eShaderRead;

		memory_barrier.src_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
		memory_barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
	}

	const auto discard_blur_view = [&](const backend::ImageView &view) {
		common::ImageMemoryBarrier memory_barrier;

		memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
		memory_barrier.new_layout      = vk::ImageLayout::eGeneral;
		memory_barrier.src_access_mask = vk::AccessFlagBits2::eNone;
		memory_barrier.dst_access_mask = vk::AccessFlagBits2::eShaderWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eComputeShader;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eComputeShader;

		command_buffer.image_memory_barrier(view, memory_barrier);
	};

	const auto read_only_blur_view = [&](const backend::ImageView &view, bool is_final) {
		common::ImageMemoryBarrier memory_barrier{};

		memory_barrier.old_layout      = vk::ImageLayout::eGeneral;
		memory_barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
		memory_barrier.src_access_mask = vk::AccessFlagBits2::eShaderWrite;
		memory_barrier.dst_access_mask = is_final ? vk::AccessFlagBits2::eNone : vk::AccessFlagBits2::eShaderRead;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eComputeShader;
		memory_barrier.dst_stage_mask  = is_final ? vk::PipelineStageFlagBits2::eBottomOfPipe : vk::PipelineStageFlagBits2::eComputeShader;

		command_buffer.image_memory_barrier(view, memory_barrier);
	};

	struct Push
	{
		uint32_t width, height;
		float    inv_width, inv_height;
		float    inv_input_width, inv_input_height;
	};

	const auto dispatch_pass = [&](const backend::ImageView &dst, const backend::ImageView &src, bool final = false) {
		discard_blur_view(dst);

		auto dst_extent = downsample_extent(dst.get_image().get_extent(), dst.get_subresource_range().baseMipLevel);
		auto src_extent = downsample_extent(src.get_image().get_extent(), dst.get_subresource_range().baseMipLevel);

		Push push{};
		push.width            = dst_extent.width;
		push.height           = dst_extent.height;
		push.inv_width        = 1.0f / static_cast<float>(push.width);
		push.inv_height       = 1.0f / static_cast<float>(push.height);
		push.inv_input_width  = 1.0f / static_cast<float>(src_extent.width);
		push.inv_input_height = 1.0f / static_cast<float>(src_extent.height);

		command_buffer.push_constants(push);
		command_buffer.bind_image(src, *linear_sampler_, 0, 0, 0);
		command_buffer.bind_image(dst, 0, 1, 0);
		command_buffer.dispatch((push.width + 7) / 8, (push.height + 7) / 8, 1);

		read_only_blur_view(dst, final);
	};

	auto               &render_frame       = render_context_.get_active_frame();
	const RenderTarget &main_render_target = render_frame.get_render_target("main_pass");

	auto &views = get_render_target()->get_views();

	command_buffer.bind_pipeline_layout(*threshold_pipeline_layout_);
	dispatch_pass(views[0], main_render_target.get_views()[0]);

	command_buffer.bind_pipeline_layout(*blur_down_pipeline_layout_);
	for (uint32_t index = 1; index < views.size(); index++)
	{
		dispatch_pass(views[index], views[index - 1]);
	}

	command_buffer.bind_pipeline_layout(*blur_up_pipeline_layout_);
	for (uint32_t index = views.size() - 2; index > 0; index--)
	{
		dispatch_pass(views[index], views[index + 1], index == 1);
	}

	// todo
	{
		common::ImageMemoryBarrier memory_barrier;
		memory_barrier.old_layout = vk::ImageLayout::eReadOnlyOptimal;
		memory_barrier.new_layout      = vk::ImageLayout::eReadOnlyOptimal;
		memory_barrier.src_access_mask = vk::AccessFlagBits2::eNone;
		memory_barrier.dst_access_mask = vk::AccessFlagBits2::eNone;

		memory_barrier.src_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
		memory_barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eBottomOfPipe;
	}
}
}        // namespace xihe::rendering
