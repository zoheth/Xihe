#pragma once

#include <vulkan/vulkan.hpp>

#include "common/logging.h"

namespace xihe
{
inline bool is_depth_only_format(vk::Format format)
{
	switch (format)
	{
	case vk::Format::eD32Sfloat:
	case vk::Format::eD16Unorm:
		return true;
	default:
		return false;
	}
}

inline vk::Format get_suitable_depth_format(vk::PhysicalDevice             physical_device,
                                            bool                           depth_only                 = false,
                                            const std::vector<vk::Format> &depth_format_priority_list = {
                                                vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm})
{
	vk::Format depth_format = vk::Format::eUndefined;

	for (auto &format : depth_format_priority_list)
	{
		if (depth_only && !is_depth_only_format(format))
		{
			continue;;
		}
		vk::FormatProperties format_properties = physical_device.getFormatProperties(format);

		if (format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			depth_format = format;
			break;
		}
	}

	if (depth_format == vk::Format::eUndefined)
	{
		throw std::runtime_error("failed to find a suitable depth format!");
	}
	LOGI("depth format: {}", vk::to_string(depth_format).c_str());

	return depth_format;
}
}
