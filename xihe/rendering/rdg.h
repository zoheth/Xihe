#pragma once
#include "backend/buffer.h"
#include "backend/image.h"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace xihe::rendering
{
typedef uint32_t RdgHandle;

class RdgPass;

enum RdgPassType
{
	kNone    = 0,
	kRaster  = 1 << 0,
	kCompute = 1 << 1,
	kMesh    = 1 << 2
};

struct RdgResourceHandle
{
	RdgHandle handle;
};
struct RdgNodeHandle
{
	RdgHandle handle;
};
struct RdgPassHandle
{
	RdgHandle handle;
};

enum RdgResourceType
{
	kInvalid    = -1,
	kBuffer     = 0,
	kTexture    = 1,
	kAttachment = 2,
	kReference  = 3,
	kSwapchain  = 4
};

struct RdgResourceInfo
{
	bool external = false;

	struct BufferData
	{
		vk::BufferUsageFlags usage;
		backend::Buffer     *buffer;
	};

	struct TextureData
	{
		vk::Format          format;
		vk::Extent3D        extent;
		vk::ImageUsageFlags usage;
		vk::ImageLayout     layout;
		backend::ImageView *image_view;
	};

	union Data
	{
		BufferData  buffer;
		TextureData texture;
	};
};

struct RdgResource
{
	RdgResourceType type;
	RdgResourceInfo info;

	RdgNodeHandle     producer;
	RdgResourceHandle output_handle;

	int32_t ref_count = 0;

	std::string name;
};

struct RdgNode
{
	int32_t ref_count = 0;

	RdgPass *pass;

	std::vector<RdgResourceHandle> inputs;
	std::vector<RdgResourceHandle> outputs;

	std::vector<RdgNodeHandle> dependencies;
};

}        // namespace xihe::rendering
