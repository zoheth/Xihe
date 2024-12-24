#include "gpu_scene.h"

#include "meshoptimizer.h"

#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"

namespace
{
glm::vec4 convert_to_vec4(const std::vector<uint8_t> &data, uint32_t offset, float padding = 1.0f)
{
	if (data.size() < offset + 3 * sizeof(float))
		throw std::runtime_error("Data size is too small for conversion to vec4.");

	float x, y, z;
	std::memcpy(&x, &data[offset], sizeof(float));
	std::memcpy(&y, &data[offset + sizeof(float)], sizeof(float));
	std::memcpy(&z, &data[offset + 2 * sizeof(float)], sizeof(float));

	return {x, y, z, padding};
}
}        // namespace

namespace xihe
{
MeshData::MeshData(const MeshPrimitiveData &primitive_data)
{
	auto pos_it    = primitive_data.attributes.find("position");
	auto normal_it = primitive_data.attributes.find("normal");
	auto uv_it     = primitive_data.attributes.find("texcoord_0");

	if (pos_it == primitive_data.attributes.end() || normal_it == primitive_data.attributes.end() || uv_it == primitive_data.attributes.end())
	{
		throw std::runtime_error("Position, Normal or UV attribute not found.");
	}

	const VertexAttributeData &pos_attr    = pos_it->second;
	const VertexAttributeData &normal_attr = normal_it->second;
	const VertexAttributeData &uv_attr     = uv_it->second;

	if (pos_attr.stride == 0 || normal_attr.stride == 0)
	{
		throw std::runtime_error("Stride for position or normal attribute is zero.");
	}
	uint32_t vertex_count = primitive_data.vertex_count;

	vertices.reserve(vertex_count);

	for (size_t i = 0; i < vertex_count; i++)
	{
		uint32_t pos_offset    = i * pos_attr.stride;
		uint32_t normal_offset = i * normal_attr.stride;
		uint32_t uv_offset     = i * uv_attr.stride;
		float    u, v;
		std::memcpy(&u, &uv_attr.data[uv_offset], sizeof(float));
		std::memcpy(&v, &uv_attr.data[uv_offset + sizeof(float)], sizeof(float));
		glm::vec4 pos    = convert_to_vec4(pos_attr.data, pos_offset, u);
		glm::vec4 normal = convert_to_vec4(normal_attr.data, normal_offset, v);
		vertices.push_back({pos, normal});
	}

	prepare_meshlets(primitive_data);
}

void MeshData::prepare_meshlets(const MeshPrimitiveData &primitive_data)
{
	std::vector<uint32_t> index_data_32;
	if (primitive_data.index_type == vk::IndexType::eUint16)
	{
		const uint16_t *index_data_16 = reinterpret_cast<const uint16_t *>(primitive_data.indices.data());
		index_data_32.resize(primitive_data.index_count);
		for (size_t i = 0; i < primitive_data.index_count; ++i)
		{
			index_data_32[i] = static_cast<uint32_t>(index_data_16[i]);
		}
	}
	else if (primitive_data.index_type == vk::IndexType::eUint32)
	{
		index_data_32.assign(
		    reinterpret_cast<const uint32_t *>(primitive_data.indices.data()),
		    reinterpret_cast<const uint32_t *>(primitive_data.indices.data()) + primitive_data.index_count);
	}

	// Use meshoptimizer to generate meshlets
	constexpr size_t max_vertices  = 64;
	constexpr size_t max_triangles = 124;
	constexpr float  cone_weight   = 0.0f;

	const size_t max_meshlets = meshopt_buildMeshletsBound(index_data_32.size(), max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> local_meshlets(max_meshlets);
	std::vector<uint32_t>        meshlet_vertex_indices(max_meshlets * max_vertices);
	std::vector<uint8_t>         meshlet_triangle_indices(max_meshlets * max_triangles * 3);

	auto vertex_positions = reinterpret_cast<const float *>(primitive_data.attributes.at("position").data.data());

	meshlet_count = meshopt_buildMeshlets(
	    local_meshlets.data(),
	    meshlet_vertex_indices.data(),
	    meshlet_triangle_indices.data(),
	    index_data_32.data(),
	    index_data_32.size(),
	    vertex_positions,
	    primitive_data.vertex_count,
	    sizeof(float) * 3,
	    max_vertices,
	    max_triangles,
	    cone_weight);

	local_meshlets.resize(meshlet_count);

	// Convert meshopt_Meshlet to our Meshlet structure
	for (size_t i = 0; i < meshlet_count; ++i)
	{
		const meshopt_Meshlet &local_meshlet = local_meshlets[i];

		Meshlet meshlet;
		meshlet.vertex_offset   = static_cast<uint32_t>(meshlet_vertices.size());
		meshlet.triangle_offset = static_cast<uint32_t>(meshlet_triangles.size());
		meshlet.vertex_count    = static_cast<uint32_t>(local_meshlet.vertex_count);
		meshlet.triangle_count  = static_cast<uint32_t>(local_meshlet.triangle_count);

		for (size_t j = 0; j < local_meshlet.vertex_count; ++j)
		{
			uint32_t vertex_index = meshlet_vertex_indices[local_meshlet.vertex_offset + j];
			meshlet_vertices.push_back(vertex_index);
		}

		size_t triangle_count = local_meshlet.triangle_count;
		size_t triangle_base  = local_meshlet.triangle_offset;

		for (size_t j = 0; j < triangle_count; ++j)
		{
			uint8_t idx0 = meshlet_triangle_indices[triangle_base + j * 3 + 0];
			uint8_t idx1 = meshlet_triangle_indices[triangle_base + j * 3 + 1];
			uint8_t idx2 = meshlet_triangle_indices[triangle_base + j * 3 + 2];

			// Pack three uint8_t indices into one uint32_t
			uint32_t packed_triangle = idx0 | (idx1 << 8) | (idx2 << 16);
			meshlet_triangles.push_back(packed_triangle);
		}

		meshopt_Bounds meshlet_bounds = meshopt_computeMeshletBounds(
		    meshlet_vertex_indices.data() + local_meshlet.vertex_offset,
		    meshlet_triangle_indices.data() + local_meshlet.triangle_offset,
		    local_meshlet.triangle_count, vertex_positions, primitive_data.vertex_count, sizeof(float) * 3);

		meshlet.center = glm::vec3(meshlet_bounds.center[0], meshlet_bounds.center[1], meshlet_bounds.center[2]);
		meshlet.radius = meshlet_bounds.radius;

		meshlet.cone_axis   = glm::vec3(meshlet_bounds.cone_axis[0], meshlet_bounds.cone_axis[1], meshlet_bounds.cone_axis[2]);
		meshlet.cone_cutoff = meshlet_bounds.cone_cutoff;

		meshlets.push_back(meshlet);
	}
}

void GpuScene::initialize(backend::Device &device, sg::Scene &scene)
{
	auto meshes = scene.get_components<sg::Mesh>();

	std::vector<MeshDraw> mesh_draws;
	std::vector<MeshInstanceDraw> instance_draws;

	std::vector<PackedVertex> packed_vertices;
	std::vector<Meshlet>      meshlets;
	std::vector<uint32_t>     meshlet_vertices;
	std::vector<uint32_t>     meshlet_triangles;
	for (const auto &mesh : meshes)
	{
		for (const auto &submesh_data : mesh->get_submeshes_data())
		{
			auto      &primitive_data = submesh_data.primitive_data;
			const auto pbr_material   = dynamic_cast<const sg::PbrMaterial *>(submesh_data.material);

			MeshDraw mesh_draw;
			mesh_draw.texture_indices                     = pbr_material->texture_indices;
			mesh_draw.base_color_factor                   = pbr_material->base_color_factor;
			mesh_draw.metallic_roughness_occlusion_factor = glm::vec4(pbr_material->metallic_factor, pbr_material->roughness_factor, 0.0f, 0.0f);
			mesh_draw.meshlet_offset                      = static_cast<uint32_t>(meshlets.size());
			mesh_draw.mesh_vertex_offset                  = static_cast<uint32_t>(meshlet_vertices.size());
			mesh_draw.mesh_triangle_offset                = static_cast<uint32_t>(meshlet_triangles.size());

			MeshData mesh_data{primitive_data};

			std::ranges::transform(mesh_data.meshlet_vertices, std::back_inserter(meshlet_vertices),
			                       [packed_vertices](const uint32_t &i) { return i + static_cast<uint32_t>(packed_vertices.size()); });

			meshlets.insert(meshlets.end(), mesh_data.meshlets.begin(), mesh_data.meshlets.end());
			packed_vertices.insert(packed_vertices.end(), mesh_data.vertices.begin(), mesh_data.vertices.end());
			// meshlet_vertices.insert(meshlet_vertices.end(), mesh_data.meshlet_vertices.begin(), mesh_data.meshlet_vertices.end());

			meshlet_triangles.insert(meshlet_triangles.end(), mesh_data.meshlet_triangles.begin(), mesh_data.meshlet_triangles.end());

			for (const auto &node : mesh->get_nodes())
			{
				MeshInstanceDraw instance_draw;
				auto node_transform = node->get_transform().get_world_matrix();
				instance_draw.model         = node_transform;
				instance_draw.model_inverse = glm::inverse(node_transform);
				instance_draw.mesh_draw_id  = static_cast<uint32_t>(mesh_draws.size());

				instance_draws.push_back(instance_draw);
			}

			mesh_draw.meshlet_count = static_cast<uint32_t>(mesh_data.meshlets.size());
			mesh_draws.push_back(mesh_draw);
		}
	}

	instance_count_ = static_cast<uint32_t>(instance_draws.size());

	{
		backend::BufferBuilder buffer_builder{packed_vertices.size() * sizeof(PackedVertex)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		global_vertex_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		global_vertex_buffer_->set_debug_name("global vertex buffer");
		global_vertex_buffer_->update(packed_vertices);
	}
	{
		backend::BufferBuilder buffer_builder{meshlets.size() * sizeof(Meshlet)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		global_meshlet_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		global_meshlet_buffer_->set_debug_name("global meshlet buffer");
		global_meshlet_buffer_->update(meshlets);
	}
	{
		backend::BufferBuilder buffer_builder{meshlet_vertices.size() * sizeof(uint32_t)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		global_meshlet_vertices_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		global_meshlet_vertices_buffer_->set_debug_name("global meshlet vertices buffer");
		global_meshlet_vertices_buffer_->update(meshlet_vertices);
	}
		{
		backend::BufferBuilder buffer_builder{meshlet_triangles.size() * sizeof(uint32_t)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		global_packed_meshlet_indices_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		global_packed_meshlet_indices_buffer_->set_debug_name("global packed meshlet indices buffer");
		global_packed_meshlet_indices_buffer_->update(meshlet_triangles);
	}
	{
		backend::BufferBuilder buffer_builder{mesh_draws.size() * sizeof(MeshDraw)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		mesh_draws_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		mesh_draws_buffer_->set_debug_name("mesh draws buffer");
		mesh_draws_buffer_->update(mesh_draws);
	}
	{
		backend::BufferBuilder buffer_builder{sizeof(uint32_t)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		draw_counts_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		draw_counts_buffer_->set_debug_name("draw counts buffer");
		draw_counts_buffer_->update(std::vector<uint32_t>{0});
	}
	{
		backend::BufferBuilder buffer_builder{instance_draws.size() * sizeof(MeshInstanceDraw)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		instance_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		instance_buffer_->set_debug_name("instance buffer");
		instance_buffer_->update(instance_draws);
	
	}
	{
		backend::BufferBuilder buffer_builder{instance_draws.size() * sizeof(MeshDrawCommand)};
		buffer_builder.with_usage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer)
		    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
		draw_command_buffer_ = std::make_unique<backend::Buffer>(device, buffer_builder);
		draw_command_buffer_->set_debug_name("draw command buffer");
		draw_command_buffer_->update(std::vector<MeshDrawCommand>(instance_draws.size()));
	}
}

backend::Buffer &GpuScene::get_instance_buffer() const
{
	if (!instance_buffer_)
	{
		throw std::runtime_error("Instance buffer is not initialized.");
	}
	return *instance_buffer_;
}

backend::Buffer &GpuScene::get_mesh_draws_buffer() const
{
	if (!mesh_draws_buffer_)
	{
		throw std::runtime_error("Mesh draws buffer is not initialized.");
	}
	return *mesh_draws_buffer_;
}

backend::Buffer & GpuScene::get_draw_command_buffer() const
{
	if (!draw_command_buffer_)
	{
		throw std::runtime_error("Draw command buffer is not initialized.");
	}
	return *draw_command_buffer_;
}

backend::Buffer &GpuScene::get_draw_counts_buffer() const
{
	if (!draw_counts_buffer_)
	{
		throw std::runtime_error("Draw counts buffer is not initialized.");
	}
	return *draw_counts_buffer_;
}

backend::Buffer &GpuScene::get_global_vertex_buffer() const
{
	if (!global_vertex_buffer_)
	{
		throw std::runtime_error("Global vertex buffer is not initialized.");
	}
	return *global_vertex_buffer_;
}

backend::Buffer &GpuScene::get_global_meshlet_buffer() const
{
	if (!global_meshlet_buffer_)
	{
		throw std::runtime_error("Global meshlet buffer is not initialized.");
	}
	return *global_meshlet_buffer_;
}

backend::Buffer &GpuScene::get_global_meshlet_vertices_buffer() const
{
	if (!global_meshlet_vertices_buffer_)
	{
		throw std::runtime_error("Global meshlet vertices buffer is not initialized.");
	}
	return *global_meshlet_vertices_buffer_;
}

backend::Buffer & GpuScene::get_global_packed_meshlet_indices_buffer() const
{
	if (!global_packed_meshlet_indices_buffer_)
	{
		throw std::runtime_error("Global packed meshlet indices buffer is not initialized.");
	}
	return *global_packed_meshlet_indices_buffer_;
}

uint32_t GpuScene::get_instance_count() const
{
	if (instance_count_ == 0)
	{
		throw std::runtime_error("Instance count is not initialized.");
	}
	return instance_count_;
}
}        // namespace xihe
