#pragma once
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
	kCompute = 1 << 1
};

struct RdgResourceHandle
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

	RdgPassHandle     producer;
	RdgResourceHandle output_handle;

	int32_t ref_count = 0;

	std::string name;
};

struct RdgNode
{
	RdgPassHandle pass_handle;
	RdgPass *pass;

	std::vector<RdgResourceHandle> inputs;
	std::vector<RdgResourceHandle> outputs;
};

}        // namespace xihe::rendering
