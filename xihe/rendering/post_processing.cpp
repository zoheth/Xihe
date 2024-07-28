#include "post_processing.h"

#include "backend/device.h"
#include "rendering/rdg_builder.h"

namespace
{
vk::Extent3D downsample_extent(const vk::Extent3D &extent, uint32_t level)
{
	return {
	    std::max(1u, extent.width >> level),
	    std::max(1u, extent.height >> level),
	    std::max(1u, extent.depth >> level)};
}



}        // namespace

namespace xihe::rendering
{
void render_blur(RdgBuilder &rdg_builder, backend::CommandBuffer &command_buffer, const backend::Sampler &sampler)
{
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
		memory_barrier.dst_access_mask = is_final ? vk::AccessFlagBits2::eShaderRead : vk::AccessFlagBits2::eShaderRead;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eComputeShader;
		memory_barrier.dst_stage_mask  = is_final ? vk::PipelineStageFlagBits2::eFragmentShader : vk::PipelineStageFlagBits2::eComputeShader;

		command_buffer.image_memory_barrier(view, memory_barrier);
	};

	struct Push
	{
		uint32_t width,           height;
		float    inv_width,       inv_height;
		float    inv_input_width, inv_input_height;
	};

	const auto dispatch_pass = [&](const backend::ImageView &dst, const backend::ImageView &src, bool final = false) {
		discard_blur_view(dst);

		const auto dst_extent = downsample_extent(dst.get_image().get_extent(), dst.get_subresource_range().baseMipLevel);
		const auto src_extent = downsample_extent(src.get_image().get_extent(), dst.get_subresource_range().baseMipLevel);

		Push push{};
		push.width            = dst_extent.width;
		push.height           = dst_extent.height;
		push.inv_width        = 1.0f / static_cast<float>(push.width);
		push.inv_height       = 1.0f / static_cast<float>(push.height);
		push.inv_input_width  = 1.0f / static_cast<float>(src_extent.width);
		push.inv_input_height = 1.0f / static_cast<float>(src_extent.height);

		command_buffer.push_constants(push);
		command_buffer.bind_image(src, sampler, 0, 0, 0);
		command_buffer.bind_image(dst, 0, 1, 0);
		command_buffer.dispatch((push.width + 7) / 8, (push.height + 7) / 8, 1);

		read_only_blur_view(dst, final);
	};
}
}
