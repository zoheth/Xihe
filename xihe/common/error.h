#pragma once

#include <stdexcept>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.hpp>

namespace xihe
{
class VulkanException : public std::runtime_error
{
  public:
	VulkanException(vk::Result result, const std::string &msg = "Vulkan error");
	VulkanException(VkResult result, const std::string &msg = "Vulkan error");

	const char *what() const noexcept override;

	VkResult result;

  private:
	std::string error_msg_;
};
}        // namespace xihe
