#pragma once

#include <vulkan/vulkan.hpp>

#include "backend/vulkan_resource.h"

namespace xihe::backend
{
struct SubpassInfo
{
	std::vector<uint32_t> input_attachments;
	std::vector<uint32_t> output_attachments;
	std::vector<uint32_t> color_resolve_attachments;

	bool                    disable_depth_stencil_attachment;
	uint32_t                depth_stencil_resolve_attachment;
	vk::ResolveModeFlagBits depth_stencil_resolve_mode;

	std::string debug_name;
};

class RenderPass : public VulkanResource<vk::RenderPass>
{
public:
};
}        // namespace xihe::backend