#version 450

precision highp float;

layout(binding = 0) uniform sampler2D i_depth;
layout(binding = 1) uniform sampler2D i_albedo;
layout(binding = 2) uniform sampler2D i_normal;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 3) uniform GlobalUniform
{
    mat4 inv_view_proj;
}
global_uniform;

#include "lighting.h"

layout(set = 0, binding = 4) uniform LightsInfo
{
	Light directional_lights[MAX_LIGHT_COUNT];
	Light point_lights[MAX_LIGHT_COUNT];
	Light spot_lights[MAX_LIGHT_COUNT];
}
lights_info;

layout(constant_id = 0) const uint DIRECTIONAL_LIGHT_COUNT = 0U;
layout(constant_id = 1) const uint POINT_LIGHT_COUNT       = 0U;
layout(constant_id = 2) const uint SPOT_LIGHT_COUNT        = 0U;


#extension GL_EXT_nonuniform_qualifier : require
layout (set = 1, binding = 10 ) uniform sampler2DShadow global_textures[];

void main()
{
	// Retrieve position from depth

	vec2 screen_uv = gl_FragCoord.xy / vec2(textureSize(i_depth, 0));

	vec4 clip = vec4(in_uv * 2.0 - 1.0, texture(i_depth, screen_uv).x, 1.0);

	highp vec4 world_w = global_uniform.inv_view_proj * clip;
	highp vec3 pos     = world_w.xyz / world_w.w;
	vec4 albedo = texture(i_albedo, screen_uv);
	// Transform from [0,1] to [-1,1]
	vec3 normal = texture(i_normal, screen_uv).xyz;
	normal      = normalize(2.0 * normal - 1.0);


	// Calculate lighting
	vec3 L = vec3(0.0);
	for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
	{
		L += apply_directional_light(lights_info.directional_lights[i], normal);
	}
	for (uint i = 0U; i < POINT_LIGHT_COUNT; ++i)
	{
		L += apply_point_light(lights_info.point_lights[i], pos, normal);
	}
	for (uint i = 0U; i < SPOT_LIGHT_COUNT; ++i)
	{
		L += apply_spot_light(lights_info.spot_lights[i], pos, normal);
	}
	vec3 ambient_color = vec3(0.2) * albedo.xyz;

	vec3 final_color = ambient_color + L * albedo.xyz;
	
	o_color = vec4(final_color, 1.0);
}