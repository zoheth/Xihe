#pragma once

#include "common/glm_common.h"

/**
 * @brief PBR material uniform for base shader
 */
struct PBRMaterialUniform
{
	glm::uvec4 texture_indices;

	glm::vec4 base_color_factor;

	float metallic_factor;

	float roughness_factor;
};

struct alignas(16) SceneUniform
{
	glm::mat4 model;

	glm::mat4 camera_view_proj;

	glm::vec3 camera_position;
};

struct alignas(16) MeshletUniform
{
	glm::vec3 center;
	float     radius;

	int8_t cone_axis[3];
	int8_t cone_cutoff;

	uint32_t data_offset;
	uint32_t mesh_index;
	uint8_t  vertex_count;
	uint8_t  triangle_count;
};

struct alignas(16) MeshletVertexPosition
{
	glm::vec3 position;
};

struct alignas(16) MeshletVertexData
{
	uint8_t   normal[4];
	uint8_t   tangent[4];
	uint16_t  tex_coord[2];
};
