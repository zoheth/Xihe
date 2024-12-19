struct Meshlet
{
	uint  vertex_offset;
	uint  triangle_offset;
	uint  vertex_count;
	uint  triangle_count;
	vec3  center;
	float radius;
	vec3  cone_axis;
	float cone_cutoff;
};

struct MeshDraw
{
	// x = diffuse index, y = roughness index, z = normal index, w = occlusion index.
	// Occlusion and roughness are encoded in the same texture
	uvec4 texture_indices;
	vec4  base_color_factor;
	vec4  metallic_roughness_occlusion_factor;

	uint meshlet_offset;
	uint meshlet_count;
};

struct MeshInstanceDraw
{
	mat4 model;
	mat4 model_inverse;

	uint mesh_draw_id;
	uint padding[3];
};

struct MeshDrawCommand
{
	uint draw_id;

	// VkDrawIndexedIndirectCommand
	uint index_count;
	uint instance_count;
	uint first_index;
	uint vertex_offset;
	uint first_instance;

	// VkDrawMeshTasksIndirectCommandEXT
	uint group_count_x;
	uint group_count_y;
	uint group_count_z;
};