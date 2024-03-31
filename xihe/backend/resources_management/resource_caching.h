#pragma once

#include <vulkan/vulkan_hash.hpp>

#include "backend/device.h"
#include "backend/descriptor_pool.h"
#include "backend/descriptor_set.h"
#include "backend/descriptor_set_layout.h"
#include "backend/framebuffer.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"
#include "resource_record.h"


namespace 
{
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
}

namespace std
{
template <typename Key, typename Value>
struct hash<std::map<Key, Value>>
{
	size_t operator()(const std::map<Key, Value>& map) const
	{
		size_t hash = 0;
		hash_combine(hash, map.size());
		for (const auto& pair : map)
		{
			hash_combine(hash, pair.first);
			hash_combine(hash, pair.second);
		}
		return hash;
	}
};

template <typename T>
struct hash<std::vector<T>>
{
	size_t operator()(const std::vector<T>& vec) const
	{
		size_t hash = 0;
		hash_combine(hash, vec.size());
		for (const auto& value : vec)
		{
			hash_combine(hash, value);
		}
		return hash;
	}
};

template <>
struct hash<xihe::LoadStoreInfo>
{
	size_t operator()(const xihe::LoadStoreInfo& info) const noexcept
	{
		size_t hash = 0;
		hash_combine(hash, info.load_op);
		hash_combine(hash, info.store_op);
		return hash;
	}
};

template <typename T>
struct hash<xihe::backend::VulkanResource<T>>
{
	size_t operator()(const xihe::backend::VulkanResource<T>& resource) const noexcept
	{
		return std::hash<T>()(resource.get_handle());
	}
};

template <>
struct hash<xihe::backend::DescriptorPool>
{
	size_t operator()(const xihe::backend::DescriptorPool &descriptor_pool) const noexcept
	{
		std::size_t hash = 0;

		hash_combine(hash, descriptor_pool.get_descriptor_set_layout());

		return hash;
	}
};



}
