#include "image.h"

#include "backend/device.h"
#include "common/error.h"

namespace
{
inline vk::ImageType find_image_type(vk::Extent3D const &extent)
{
	switch (!!extent.width + !!extent.height + (1 < extent.depth))
	{
		case 1:
			return vk::ImageType::e1D;
		case 2:
			return vk::ImageType::e2D;
		case 3:
			return vk::ImageType::e3D;
		default:
			throw std::runtime_error("No image type found.");
			return vk::ImageType();
	}
}
}        // namespace

namespace xihe::backend
{
Image::Image(Device &device, vk::Image handle, const vk::Extent3D &extent, vk::Format format, vk::ImageUsageFlags image_usage, vk::SampleCountFlagBits sample_count) :
    VulkanResource{handle, &device}, type_{find_image_type(extent)}, extent_{extent}, format_{format}, usage_{image_usage}, sample_count_{sample_count}
{
	subresource_.mipLevel   = 1;
	subresource_.arrayLayer = 1;
}

Image::Image(Device &device, const vk::Extent3D &extent, vk::Format format, vk::ImageUsageFlags image_usage, VmaMemoryUsage memory_usage, vk::SampleCountFlagBits sample_count, uint32_t mip_levels, uint32_t array_layers, vk::ImageTiling tiling, vk::ImageCreateFlags flags, uint32_t num_queue_families, const uint32_t *queue_families) :
    VulkanResource{nullptr, &device}, type_{find_image_type(extent)}, extent_{extent}, format_{format}, usage_{image_usage}, sample_count_{sample_count}, array_layer_count_{array_layers}, tiling_{tiling}
{
	assert(0 < mip_levels && "HPPImage should have at least one level");
	assert(0 < array_layers && "HPPImage should have at least one layer");

	subresource_.mipLevel   = mip_levels;
	subresource_.arrayLayer = array_layers;

	vk::ImageCreateInfo image_info(flags, type_, format, extent, mip_levels, array_layers, sample_count, tiling, image_usage);

	if (num_queue_families != 0)
	{
		image_info.sharingMode           = vk::SharingMode::eConcurrent;
		image_info.queueFamilyIndexCount = num_queue_families;
		image_info.pQueueFamilyIndices   = queue_families;
	}

	VmaAllocationCreateInfo memory_info{};
	memory_info.usage = memory_usage;

	if (image_usage & vk::ImageUsageFlagBits::eTransientAttachment)
	{
		memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}

	auto result = vmaCreateImage(device.get_memory_allocator(),
	                             reinterpret_cast<VkImageCreateInfo const *>(&image_info),
	                             &memory_info,
	                             const_cast<VkImage *>(reinterpret_cast<VkImage const *>(&get_handle())),
	                             &memory_,
	                             nullptr);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create HPPImage"};
	}
}

vk::Extent3D Image::get_extent() const
{
	return extent_;
}

vk::ImageType Image::get_type() const
{
	return type_;
}

vk::Format Image::get_format() const
{
	return format_;
}

vk::ImageUsageFlags Image::get_usage() const
{
	return usage_;
}

vk::ImageTiling Image::get_tiling() const
{
	return tiling_;
}

vk::SampleCountFlagBits Image::get_sample_count() const
{
	return sample_count_;
}

VmaAllocation Image::get_memory() const
{
	return memory_;
}

vk::ImageSubresource Image::get_subresource() const
{
	return subresource_;
}

std::unordered_set<ImageView *> &Image::get_views()
{
	return views_;
}

uint32_t Image::get_array_layer_count() const
{
	return array_layer_count_;
}
}        // namespace xihe::backend
