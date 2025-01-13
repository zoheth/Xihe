#version 450

layout (location = 0) in vec3 inUVW;
layout (location = 0) out vec4 outColor;

layout (binding = 2) uniform samplerCube samplerEnv;


void main() 
{
	vec3 color = textureLod(samplerEnv, inUVW, 1.5).rgb;	
	outColor = vec4(color * 1.0, 1.0);
}