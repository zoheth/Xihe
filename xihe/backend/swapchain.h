#pragma once

#include <set>
#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;

struct SwapchainProperties
{
	vk::SwapchainKHR                old_swapchain;
	uint32_t                        image_count{3};
	vk::Extent2D                    extent;
	vk::SurfaceFormatKHR            surface_format;
	uint32_t                        array_layers;
	vk::ImageUsageFlags             image_usage;
	vk::SurfaceTransformFlagBitsKHR pre_transform;
	vk::CompositeAlphaFlagBitsKHR   composite_alpha;
	vk::PresentModeKHR              present_mode;
};

class Swapchain
{
  public:
	Swapchain(Swapchain &old_swapchain, const vk::Extent2D &extent);

	Swapchain(Swapchain &old_swapchain, const uint32_t image_count);

	Swapchain(Device                                  &device,
	          vk::SurfaceKHR                           surface,
	          const vk::PresentModeKHR                 present_mode,
	          const std::vector<vk::PresentModeKHR>   &present_mode_priority_list   = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
	          const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list = {{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
	                                                                                   {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}},
	          const vk::Extent2D                      &extent                       = {},
	          const uint32_t                           image_count                  = 3,
	          const vk::SurfaceTransformFlagBitsKHR    transform                    = vk::SurfaceTransformFlagBitsKHR::eIdentity,
	          const std::set<vk::ImageUsageFlagBits>  &image_usage_flags            = {vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc},
	          vk::SwapchainKHR                         old_swapchain                = nullptr);

	~Swapchain();

  private:
	Device &device_;

	vk::SurfaceKHR   surface_;
	vk::SwapchainKHR handle_;

	std::vector<vk::Image>            images_;
	std::vector<vk::SurfaceFormatKHR> surface_formats_;
	std::vector<vk::PresentModeKHR>   present_modes_;

	SwapchainProperties properties_;

	std::vector<vk::PresentModeKHR>   present_mode_priority_list_;
	std::vector<vk::SurfaceFormatKHR> surface_format_priority_list_;

	std::set<vk::ImageUsageFlagBits> image_usage_flags_;
};
}        // namespace xihe::backend
