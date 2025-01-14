#include "asset_loader.h"

#include "components/image.h"

namespace xihe
{
void upload_image_to_gpu(backend::CommandBuffer &command_buffer, const backend::Buffer &staging_buffer, sg::Image &image, vk::ImageLayout final_layout, vk::PipelineStageFlags2 final_stage, vk::AccessFlags2 final_access)
{
	image.clear_data();
	{
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
		memory_barrier.new_layout      = vk::ImageLayout::eTransferDstOptimal;
		memory_barrier.dst_access_mask = vk::AccessFlagBits2::eTransferWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eHost;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;
		command_buffer.image_memory_barrier(image.get_vk_image_view(), memory_barrier);
	}
	auto                            &mipmaps = image.get_mipmaps();
	std::vector<vk::BufferImageCopy> buffer_copy_regions(mipmaps.size());
	for (size_t i = 0; i < mipmaps.size(); ++i)
	{
		auto &mipmap                          = mipmaps[i];
		auto &copy_region                     = buffer_copy_regions[i];
		copy_region.bufferOffset              = mipmap.offset;
		copy_region.imageSubresource          = image.get_vk_image_view().get_subresource_layers();
		copy_region.imageSubresource.mipLevel = mipmap.level;
		copy_region.imageExtent               = mipmap.extent;
	}

	command_buffer.copy_buffer_to_image(staging_buffer, image.get_vk_image(), buffer_copy_regions);

	{
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eTransferDstOptimal;
		memory_barrier.new_layout      = final_layout;
		memory_barrier.src_access_mask = vk::AccessFlagBits2::eTransferWrite;
		memory_barrier.dst_access_mask = final_access;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;
		memory_barrier.dst_stage_mask  = final_stage;
		command_buffer.image_memory_barrier(image.get_vk_image_view(), memory_barrier);
	}
}

AssetLoader::AssetLoader(backend::Device &device) :
    device_{device}
{}

sg::Texture *AssetLoader::load_texture_cube(sg::Scene &scene, const std::string &name, const std::string &file_name) const
{
	auto         texture     = std::make_unique<sg::Texture>(name);
	sg::Texture *texture_ptr = texture.get();

	auto image = sg::Image::load(file_name, file_name, sg::Image::kColor);

	image->create_vk_image(device_, vk::ImageViewType::eCube, vk::ImageCreateFlagBits::eCubeCompatible);

	backend::CommandBuffer &command_buffer = device_.request_command_buffer();

	command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	backend::Buffer stage_buffer = backend::Buffer::create_staging_buffer(device_, image->get_data());

	upload_image_to_gpu(command_buffer, stage_buffer, *image);

	command_buffer.end();

	const auto &queue = device_.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
	queue.submit(command_buffer, device_.request_fence());

	device_.get_fence_pool().wait();
	device_.get_fence_pool().reset();
	device_.get_command_pool().reset_pool();
	device_.wait_idle();

	texture->set_image(*image);
	const uint32_t mip_levels = static_cast<uint32_t>(image->get_mipmaps().size());

	scene.add_component(std::move(image));

	vk::SamplerCreateInfo sampler_info{};

	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.mipmapMode   = vk::SamplerMipmapMode::eLinear;
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.mipLodBias   = 0.0f;
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod        = static_cast<float>(mip_levels);
	sampler_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;

	backend::Sampler vk_sampler{device_, sampler_info};
	vk_sampler.set_debug_name(file_name + "_sampler");

	auto sampler = std::make_unique<sg::Sampler>(file_name + "_sampler", std::move(vk_sampler));

	texture->set_sampler(*sampler);

	scene.add_component(std::move(sampler));

	scene.add_component(std::move(texture));

	return texture_ptr;
}
}        // namespace xihe
