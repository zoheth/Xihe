#pragma once
#include "backend/image.h"
#include "backend/vulkan_resource.h"

namespace xihe::backend
{
class ImageView : public VulkanResource<vk::ImageView>
{
  public:
	ImageView(Image            &image,
	          vk::ImageViewType view_type,
	          vk::Format        format           = vk::Format::eUndefined,
	          uint32_t          base_mip_level   = 0,
	          uint32_t          base_array_layer = 0,
	          uint32_t          n_mip_levels     = 0,
	          uint32_t          n_array_layers   = 0);

	ImageView(ImageView &) = delete;

	ImageView(ImageView &&other);

	~ImageView() override;

	vk::Format                 get_format() const;
	Image const               &get_image() const;
	void                       set_image(Image &image);
	vk::ImageSubresourceLayers get_subresource_layers() const;
	vk::ImageSubresourceRange  get_subresource_range() const;

  private:
	Image                    *image_{nullptr};
	vk::Format                format_;
	vk::ImageSubresourceRange subresource_range_;
};
}        // namespace xihe::backend
