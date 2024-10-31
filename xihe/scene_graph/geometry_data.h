#pragma once
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>

namespace xihe
{

struct VertexAttribute
{
	vk::Format format = vk::Format::eUndefined;
	std::uint32_t stride = 0;
	std::uint32_t offset = 0;
};

struct VertexAttributeData
{
	// std::string          name;
	vk::Format           format;
	uint32_t             stride;
	std::vector<uint8_t> data;
};

struct MeshPrimitiveData
{
	std::string                      name;
	uint32_t                         vertex_count;
	std::unordered_map<std::string, VertexAttributeData> attributes;
	std::vector<uint8_t>             indices;
	vk::IndexType                    index_type;
	uint32_t                         index_count;
};
}