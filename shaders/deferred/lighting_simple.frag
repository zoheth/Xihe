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


layout(set = 0, binding = 5) uniform ShadowUniform {
	vec4 far_d;
    mat4 light_matrix[SHADOW_MAP_CASCADE_COUNT];
	uint shadowmap_first_index;
} shadow_uniform;


#extension GL_EXT_nonuniform_qualifier : require
layout (set = 1, binding = 10 ) uniform sampler2DShadow global_textures[];

float calculate_shadow(highp vec3 pos, uint i)
{
	vec4 projected_coord = shadow_uniform.light_matrix[i] * vec4(pos, 1.0);
	projected_coord /= projected_coord.w;
	projected_coord.xy = 0.5 * projected_coord.xy + 0.5;

	if (projected_coord.x < 0.0 || projected_coord.x > 1.0 ||
        projected_coord.y < 0.0 || projected_coord.y > 1.0)
    {
        return 1.0;
    }

	float shadow = 0.0;
    int samples = 0;

    const int kernel_size = 5;
    const float shadow_map_resolution = 2048.0;
    const float texel_size = 1.0 / shadow_map_resolution;
    const float offset = texel_size;

	const float bias = 0.005;

    for(int x = -kernel_size / 2; x <= kernel_size / 2; x++) {
        for(int y = -kernel_size / 2; y <= kernel_size / 2; y++) {
            vec2 tex_offset = vec2(float(x), float(y)) * offset;
            float shadow_sample = texture(
                global_textures[nonuniformEXT(shadow_uniform.shadowmap_first_index + i)],
                vec3(projected_coord.xy + tex_offset, projected_coord.z - bias)
            );
            shadow += shadow_sample;
            samples++;
        }
    }

    shadow /= float(samples);
    return shadow;
}

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