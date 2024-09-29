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

std::unique_ptr<backend::Sampler> get_linear_sampler(backend::Device &device)
{
	auto sampler_info         = vk::SamplerCreateInfo{};
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	return std::make_unique<backend::Sampler>(device, sampler_info);
}


void render_blur(backend::CommandBuffer &command_buffer, ComputeRdgPass& rdg_pass)
{
	backend::Sampler *sampler = rdg_pass.get_pass_info().outputs[0].sampler;
	assert(sampler);
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
		command_buffer.bind_image(src, *sampler, 0, 0, 0);
		command_buffer.bind_image(dst, 0, 1, 0);
		command_buffer.dispatch((push.width + 7) / 8, (push.height + 7) / 8, 1);

		read_only_blur_view(dst, final);
	};

	auto &views = rdg_pass.get_render_target()->get_views();
	auto &hdr_view = *rdg_pass.get_input_image_views()[0];

	command_buffer.bind_pipeline_layout(*rdg_pass.get_pipeline_layouts()[0]);
	dispatch_pass(views[0], hdr_view);

	command_buffer.bind_pipeline_layout(*rdg_pass.get_pipeline_layouts()[1]);
	for (uint32_t index = 1; index < views.size(); index++)
	{
		dispatch_pass(views[index], views[index - 1]);
	}

	command_buffer.bind_pipeline_layout(*rdg_pass.get_pipeline_layouts()[2]);
	for (uint32_t index = views.size() - 2; index > 0; index--)
	{
		dispatch_pass(views[index], views[index + 1], index == 1);
	}
}
}
