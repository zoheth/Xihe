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
Image ImageBuilder::build(Device &device) const
{
	return Image(device, *this);
}

ImagePtr ImageBuilder::build_unique(Device &device) 
{
	return std::make_unique<Image>(device, *this);
}

Image::Image(Device                 &device,
             vk::Image               handle,
             const vk::Extent3D     &extent,
             vk::Format              format,
             vk::ImageUsageFlags     image_usage,
             vk::SampleCountFlagBits sample_count) :
    Allocated{handle, &device}
{
	create_info_.samples     = sample_count;
	create_info_.format      = format;
	create_info_.extent      = extent;
	create_info_.imageType   = find_image_type(extent);
	create_info_.arrayLayers = 1;
	create_info_.mipLevels   = 1;
	subresource_.mipLevel    = 1;
	subresource_.arrayLayer  = 1;
}

Image::Image(Device &device, ImageBuilder const &builder) :
    Allocated{builder.allocation_create_info, nullptr, &device}, create_info_{builder.create_info}
{
	get_handle()            = create_image(create_info_);
	subresource_.arrayLayer = create_info_.arrayLayers;
	subresource_.mipLevel   = create_info_.mipLevels;
	if (!builder.debug_name.empty())
	{
		set_debug_name(builder.debug_name);
	}
}

Image::Image(Device                 &device,
             const vk::Extent3D     &extent,
             vk::Format              format,
             vk::ImageUsageFlags     image_usage,
             VmaMemoryUsage          memory_usage,
             vk::SampleCountFlagBits sample_count,
             uint32_t mip_levels, uint32_t array_layers,
             vk::ImageTiling      tiling,
             vk::ImageCreateFlags flags,
             uint32_t             num_queue_families,
             const uint32_t      *queue_families) :
    Image{device,
          ImageBuilder{extent}
              .with_format(format)
              .with_mip_levels(mip_levels)
              .with_array_layers(array_layers)
              .with_sample_count(sample_count)
              .with_tiling(tiling)
              .with_flags(flags)
              .with_usage(image_usage)
              .with_queue_families(num_queue_families, queue_families)}
{
}

Image::Image(Image &&other) noexcept:
    Allocated{std::move(other)},
    create_info_{std::exchange(other.create_info_, {})},
    subresource_{std::exchange(other.subresource_, {})},
    views_(std::exchange(other.views_, {}))
{
	// Update image views references to this image to avoid dangling pointers
	for (auto &view : views_)
	{
		view->set_image(*this);
	}
}

Image::~Image()
{
	destroy_image(get_handle());
}

uint8_t *Image::map()
{
	if (create_info_.tiling != vk::ImageTiling::eLinear)
	{
		LOGW("Mapping image memory that is not linear");
	}
	return Allocated::map();
}

const vk::Extent3D &Image::get_extent() const
{
	return create_info_.extent;
}

vk::ImageType Image::get_type() const
{
	return create_info_.imageType;
}

vk::Format Image::get_format() const
{
	return create_info_.format;
}

vk::ImageUsageFlags Image::get_usage() const
{
	return create_info_.usage;
}

vk::ImageTiling Image::get_tiling() const
{
	return create_info_.tiling;
}

vk::SampleCountFlagBits Image::get_sample_count() const
{
	return create_info_.samples;
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
	return create_info_.arrayLayers;
}
}        // namespace xihe::backend
