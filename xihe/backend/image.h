#pragma once
#include <vk_mem_alloc.h>

#include "allocated.h"
#include "vulkan_resource.h"

#include <unordered_set>

namespace xihe::backend
{
class Device;
class ImageView;
class Image;
using ImagePtr = std::unique_ptr<Image>;

struct ImageBuilder : public allocated::Builder<ImageBuilder, vk::ImageCreateInfo>
{
  private:
	using Parent = allocated::Builder<ImageBuilder, vk::ImageCreateInfo>;

  public:
	ImageBuilder(vk::Extent3D extent) :
	    Parent(vk::ImageCreateInfo({}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, extent, 1, 1))
	{
	}

	ImageBuilder(vk::Extent2D const &extent) :
	    ImageBuilder(vk::Extent3D{extent.width, extent.height, 1})
	{
	}

	ImageBuilder(uint32_t width, uint32_t height = 1, uint32_t depth = 1) :
	    ImageBuilder(vk::Extent3D{width, height, depth})
	{
	}

	ImageBuilder &with_format(vk::Format format)
	{
		create_info.format = format;
		return *this;
	}

	ImageBuilder &with_image_type(vk::ImageType type)
	{
		create_info.imageType = type;
		return *this;
	}

	ImageBuilder &with_array_layers(uint32_t layers)
	{
		create_info.arrayLayers = layers;
		return *this;
	}

	ImageBuilder &with_mip_levels(uint32_t levels)
	{
		create_info.mipLevels = levels;
		return *this;
	}

	ImageBuilder &with_sample_count(vk::SampleCountFlagBits sample_count)
	{
		create_info.samples = sample_count;
		return *this;
	}

	ImageBuilder &with_tiling(vk::ImageTiling tiling)
	{
		create_info.tiling = tiling;
		return *this;
	}

	ImageBuilder &with_usage(vk::ImageUsageFlags usage)
	{
		create_info.usage = usage;
		return *this;
	}

	ImageBuilder &with_flags(vk::ImageCreateFlags flags)
	{
		create_info.flags = flags;
		return *this;
	}

	ImageBuilder &with_implicit_sharing_mode()
	{
		if (create_info.queueFamilyIndexCount != 0)
		{
			create_info.sharingMode = vk::SharingMode::eConcurrent;
		}
		return *this;
	}

	Image    build(Device &device) const;
	ImagePtr build_unique(Device &device);
};

class Image : public allocated::Allocated<vk::Image>
{
  public:
	Image(Device                 &device,
	      vk::Image               handle,
	      const vk::Extent3D     &extent,
	      vk::Format              format,
	      vk::ImageUsageFlags     image_usage,
	      vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

	Image(Device             &device,
	      ImageBuilder const &builder);

	Image(Device                 &device,
	      const vk::Extent3D     &extent,
	      vk::Format              format,
	      vk::ImageUsageFlags     image_usage,
	      VmaMemoryUsage          memory_usage,
	      vk::SampleCountFlagBits sample_count       = vk::SampleCountFlagBits::e1,
	      uint32_t                mip_levels         = 1,
	      uint32_t                array_layers       = 1,
	      vk::ImageTiling         tiling             = vk::ImageTiling::eOptimal,
	      vk::ImageCreateFlags    flags              = {},
	      uint32_t                num_queue_families = 0,
	      const uint32_t         *queue_families     = nullptr);

	Image(const Image &) = delete;

	Image(Image &&other) noexcept;

	~Image() override;

	uint8_t *map();

	vk::ImageType                    get_type() const;
	const vk::Extent3D              &get_extent() const;
	vk::Format                       get_format() const;
	vk::SampleCountFlagBits          get_sample_count() const;
	vk::ImageUsageFlags              get_usage() const;
	vk::ImageTiling                  get_tiling() const;
	vk::ImageSubresource             get_subresource() const;
	uint32_t                         get_array_layer_count() const;
	std::unordered_set<ImageView *> &get_views();

  private:
	vk::ImageCreateInfo             create_info_;
	vk::ImageSubresource            subresource_;
	std::unordered_set<ImageView *> views_;
};
}        // namespace xihe::backend
