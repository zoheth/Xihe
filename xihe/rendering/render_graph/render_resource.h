#pragma once

#include "backend/image_view.h"
#include "vulkan/vulkan.hpp"

namespace xihe::rendering
{

enum class PassType
{
	kNone    = 0,
	kRaster  = 1 << 0,
	kMesh    = 1 << 1,
	kCompute = 1 << 2,
};

enum class ResourceUsage
{
	// Buffer read operations
	kVertexBuffer,
	kIndexBuffer,
	kUniformBuffer,
	kStorageBufferRead,

	// Buffer write operations
	kStorageBufferWrite,

	// Buffer read-write operations
	kStorageBufferReadWrite,

	// Image read operations
	kSampledImage,
	kStorageImageRead,

	// Image write operations
	kStorageImageWrite,

	// Image read-write operations
	kStorageImageReadWrite,

	// Attachment operations
	kColorAttachment,
	kDepthStencilAttachment
};

struct ResourceUsageState
{
	vk::ImageLayout         layout{vk::ImageLayout::eUndefined};
	vk::PipelineStageFlags2 stage_mask{};
	vk::AccessFlags2        access_mask{};
};

bool is_image_resource(ResourceUsage usage);

void update_resource_state(ResourceUsage usage, PassType pass_type, ResourceUsageState &state);

}        // namespace xihe::rendering
