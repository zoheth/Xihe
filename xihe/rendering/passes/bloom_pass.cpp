#include "bloom_pass.h"

namespace xihe::rendering
{
namespace 
{
vk::SamplerCreateInfo get_linear_sampler()
{
	auto sampler_info         = vk::SamplerCreateInfo{};
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	return sampler_info;
}

vk::Extent3D downsample_extent(const vk::Extent3D &extent, uint32_t level)
{
	return {
	    std::max(1u, extent.width >> level),
	    std::max(1u, extent.height >> level),
	    std::max(1u, extent.depth >> level)};
}
}

void BloomScreenPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &src= input_bindables[0].image_view();
	auto &dst = input_bindables[1].image_view();

	const auto dst_extent = downsample_extent(dst.get_image().get_extent(), dst.get_subresource_range().baseMipLevel);
	const auto src_extent = downsample_extent(src.get_image().get_extent(), dst.get_subresource_range().baseMipLevel);

	Push push{};
	push.width            = dst_extent.width;
	push.height           = dst_extent.height;
	push.inv_width        = 1.0f / static_cast<float>(push.width);
	push.inv_height       = 1.0f / static_cast<float>(push.height);
	push.inv_input_width  = 1.0f / static_cast<float>(src_extent.width);
	push.inv_input_height = 1.0f / static_cast<float>(src_extent.height);

	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	command_buffer.push_constants(push);
	command_buffer.bind_image(src, resource_cache.request_sampler(get_linear_sampler()), 0, 0, 0);
	command_buffer.bind_image(dst, 0, 1, 0);
	command_buffer.dispatch((push.width + 7) / 8, (push.height + 7) / 8, 1);
}
}
