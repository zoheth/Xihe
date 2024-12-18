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

struct MeshInstanceDraw
{
	mat4 model;
	mat4 model_inverse;

	uint mesh_id;
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