#include "error.h"

#include "strings.h"

namespace xihe
{
VulkanException::VulkanException(VkResult result, const std::string &msg) :
    std::runtime_error{msg},
    result{result}
{
	error_msg_ = std::string(std::runtime_error::what()) + std::string{" : "} + to_string(result);
}

const char *VulkanException::what() const noexcept
{
	return error_msg_.c_str();
}
}        // namespace xihe
