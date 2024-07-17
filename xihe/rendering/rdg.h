#pragma once
#include <cstdint>
#include <string>

namespace xihe::rendering
{
typedef uint32_t RdgHandle;

enum RdgPassType
{
	kNone    = 0,
	kRaster  = 1 << 0,
	kCompute = 1 << 1
};

struct RdgResourceHandle
{
	RdgHandle handle;
};
struct RdgNodeHandle
{
	RdgHandle handle;
};

enum RdgResourceType
{
	kInvalid    = -1,
	kBuffer     = 0,
	kTexture    = 1,
	kAttachment = 2,
	kReference  = 3
};

struct RdgResourceInfo
{
	bool external = false;
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
};

}        // namespace xihe::rendering
