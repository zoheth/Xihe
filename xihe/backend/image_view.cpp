#include "image_view.h"

#include <vulkan/vulkan_format_traits.hpp>

#include "backend/device.h"

namespace xihe::backend
{
ImageView::ImageView(Image &image, vk::ImageViewType view_type, vk::Format format, uint32_t base_mip_level, uint32_t base_array_layer, uint32_t n_mip_levels, uint32_t n_array_layers) :
    VulkanResource{nullptr, &image.get_device()},
    image_{&image},
    format_{format}
{
	if (format == vk::Format::eUndefined)
	{
		format_ = format = image_->get_format();
	}

	subresource_range_ =
	    vk::ImageSubresourceRange((std::string(vk::componentName(format, 0)) == "D") ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
	                              base_mip_level,
	                              n_mip_levels == 0 ? image_->get_subresource().mipLevel : n_mip_levels,
	                              base_array_layer,
	                              n_array_layers == 0 ? image_->get_subresource().arrayLayer : n_array_layers);

	vk::ImageViewCreateInfo image_view_create_info({}, image_->get_handle(), view_type, format, {}, subresource_range_);

	set_handle(get_device().get_handle().createImageView(image_view_create_info));

	// Register this image view to its image
	// in order to be notified when it gets moved
	image_->get_views().emplace(this);
}

ImageView::ImageView(ImageView &&other):
    VulkanResource{std::move(other)},
    image_{other.image_},
    format_{other.format_},
    subresource_range_{other.subresource_range_}
{
	auto &views = image_->get_views();
	views.erase(&other);
	views.emplace(this);
}

ImageView::~ImageView()
{
	if (has_device())
	{
		get_device().get_handle().destroyImageView(get_handle());
	}
}

vk::Format ImageView::get_format() const
{
	return format_;
}

Image const &ImageView::get_image() const
{
	assert(image_ && "Image view has no image");
	return *image_;
}

void ImageView::set_image(Image &image)
{
	image_ = &image;
}

vk::ImageSubresourceLayers ImageView::get_subresource_layers() const
{
	return {subresource_range_.aspectMask, subresource_range_.baseMipLevel,
	        subresource_range_.baseArrayLayer, subresource_range_.layerCount};
}

vk::ImageSubresourceRange ImageView::get_subresource_range() const
{
	return subresource_range_;
}
}        // namespace xihe::backend
