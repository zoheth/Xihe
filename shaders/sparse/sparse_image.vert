#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord_0;

layout(set = 0, binding = 0) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
} global_uniform;

layout (location = 0) out vec2 o_uv;

void main(void)
{
    o_uv = texcoord_0;
    gl_Position = global_uniform.view_proj * global_uniform.model * vec4(position, 0.0, 1.0);
}