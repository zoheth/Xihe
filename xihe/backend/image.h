#pragma once
#include <vk_mem_alloc.h>

#include "vulkan_resource.h"

#include <unordered_set>

namespace xihe::backend
{
class Device;
class ImageView;

class Image : public VulkanResource<vk::Image>
{
  public:
	Image(Device                 &device,
	      vk::Image               handle,
	      const vk::Extent3D     &extent,
	      vk::Format              format,
	      vk::ImageUsageFlags     image_usage,
	      vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

  private:
	VmaAllocation memory_{VK_NULL_HANDLE};

	vk::ImageType           type_;
	vk::Extent3D            extent_;
	vk::Format              format_;
	vk::ImageUsageFlags     usage_;
	vk::SampleCountFlagBits sample_count_;
	vk::ImageTiling         tiling_;
	vk::ImageSubresource    subresource_;

	uint32_t array_layer_count_{0};

	std::unordered_set<ImageView *> views_;

	uint8_t *mapped_data_{nullptr};
	bool     mapped_{false};
};
}        // namespace xihe::backend
