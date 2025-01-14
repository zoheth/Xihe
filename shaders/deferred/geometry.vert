#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord_0;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(set = 0, binding = 1) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
} global_uniform;

layout (location = 0) out vec4 o_pos;
layout (location = 1) out vec2 o_uv;
layout (location = 2) out vec3 o_normal;
layout (location = 3) out vec3 o_tangent;

void main(void)
{
    o_pos = global_uniform.model * vec4(position, 1.0);

    o_uv = texcoord_0;

    // o_normal = mat3(global_uniform.model) * normal;

    mat3 normal_matrix = mat3(transpose(inverse(global_uniform.model)));
    o_normal = normalize(normal_matrix * normal);
    o_tangent = normalize(normal_matrix * tangent);

    gl_Position = global_uniform.view_proj * o_pos;
}