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

	vk::Extent3D            get_extent() const;
	vk::ImageType           get_type() const;
	vk::Format              get_format() const;
	vk::ImageUsageFlags     get_usage() const;
	vk::SampleCountFlagBits get_sample_count() const;

	vk::ImageSubresource get_subresource() const;
	std::unordered_set<ImageView *> &get_views();

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
