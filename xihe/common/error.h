#pragma once

#include <stdexcept>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.hpp>

#include "common/helpers.h"
#include "common/logging.h"

#if defined(__clang__)
// CLANG ENABLE/DISABLE WARNING DEFINITION
#	define XH_DISABLE_WARNINGS()                             \
		_Pragma("clang diagnostic push")                        \
		    _Pragma("clang diagnostic ignored \"-Wall\"")       \
		        _Pragma("clang diagnostic ignored \"-Wextra\"") \
		            _Pragma("clang diagnostic ignored \"-Wtautological-compare\"")

#	define XH_ENABLE_WARNINGS() \
		_Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
// GCC ENABLE/DISABLE WARNING DEFINITION
#	define XH_DISABLE_WARNINGS()                           \
		_Pragma("GCC diagnostic push")                        \
		    _Pragma("GCC diagnostic ignored \"-Wall\"")       \
		        _Pragma("GCC diagnostic ignored \"-Wextra\"") \
		            _Pragma("GCC diagnostic ignored \"-Wtautological-compare\"")

#	define XH_ENABLE_WARNINGS() \
		_Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
// MSVC ENABLE/DISABLE WARNING DEFINITION
#	define XH_DISABLE_WARNINGS() \
		__pragma(warning(push, 0))

#	define XH_ENABLE_WARNINGS() \
		__pragma(warning(pop))
#endif

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

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			LOGE("Detected Vulkan error: {}", xihe::to_string(err)); \
			abort();                                                \
		}                                                           \
	} while (0)

#define ASSERT_VK_HANDLE(handle)        \
	do                                  \
	{                                   \
		if ((handle) == VK_NULL_HANDLE) \
		{                               \
			LOGE("Handle is NULL");     \
			abort();                    \
		}                               \
	} while (0)

}        // namespace xihe
