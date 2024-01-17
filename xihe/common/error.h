#pragma once

#include <stdexcept>

#include <vulkan/vulkan_core.h>

namespace xihe
{
class VulkanException : public std::runtime_error
{
	public:
	VulkanException(VkResult result, const std::string &msg = "Vulkan error");

	const char *what() const noexcept override;

	VkResult result;

private:
	std::string error_msg_;
};
}
