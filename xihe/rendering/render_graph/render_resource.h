#pragma once

#include "vulkan/vulkan.hpp"

namespace xihe::rendering
{
struct ResourceDesc
{
};

struct ResourceState
{
	vk::ImageLayout         layout;
	vk::PipelineStageFlags2 stages;
	vk::AccessFlags2        access;
	uint32_t                queue_family_index;
};

template<typename T>
class ResourceHandle
{
	uint32_t id_;
	ResourceState state_;

public:
	const ResourceState &get_state() const
	{
		return state_;
	}

	void transition(const ResourceState &state);
};

class ResourceManager
{
public:

};
}        // namespace xihe::rendering
