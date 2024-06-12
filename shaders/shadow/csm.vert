#version 450

layout(location = 0) in vec3 position;

layout(set = 0, binding = 1) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
} global_uniform;

void main(void)
{
    vec4 pos = global_uniform.model * vec4(position, 1.0);
    gl_Position = global_uniform.view_proj * pos;
}