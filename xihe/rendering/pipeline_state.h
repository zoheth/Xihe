#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

namespace xihe
{
struct VertexInputState
{
	std::vector<vk::VertexInputBindingDescription> bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
};

struct InputAssemblyState
{
	vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
	bool primitive_restart_enable {VK_FALSE};
};

struct RasterizationState
{
	bool depth_clamp_enable {VK_FALSE};
	bool rasterizer_discard_enable {VK_FALSE};

	vk::PolygonMode polygon_mode {vk::PolygonMode::eFill};
	vk::CullModeFlags cull_mode {vk::CullModeFlagBits::eBack};

	vk::FrontFace front_face {vk::FrontFace::eCounterClockwise};

	vk::Bool32 depth_bias_enable {VK_FALSE};
};

struct ViewportState
{
	uint32_t viewport_count {1};
	uint32_t scissor_count {1};
};



}
