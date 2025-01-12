#version 450

layout(location = 0) in vec3 position;

layout (binding = 0) uniform UBO 
{
	mat4 mvp;
} ubo;

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = position;
	gl_Position = ubo.mvp * vec4(position.xyz, 1.0);
}
