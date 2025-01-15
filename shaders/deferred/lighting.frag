#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

precision highp float;

layout(binding = 0) uniform sampler2D i_depth;
layout(binding = 1) uniform sampler2D i_albedo;
layout(binding = 2) uniform sampler2D i_normal;
layout(binding = 9) uniform sampler2D i_emissive;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 3) uniform GlobalUniform
{
    mat4 inv_view_proj;
	vec3 camera_pos;
}
global_uniform;

layout (set = 0, binding = 11) uniform samplerCube samplerIrradiance;
layout (set = 0, binding = 12) uniform samplerCube prefilteredMap;
layout (set = 0, binding = 13) uniform sampler2D samplerBRDFLUT;

#include "pbr_lighting.h"

layout(set = 0, binding = 4) uniform LightsInfo
{
	Light directional_lights[MAX_LIGHT_COUNT];
	Light point_lights[MAX_LIGHT_COUNT];
	Light spot_lights[MAX_LIGHT_COUNT];
}
lights_info;

layout(set = 0, binding = 5) uniform ShadowUniform {
    vec4 far_d;
    mat4 light_matrix[SHADOW_MAP_CASCADE_COUNT];
} shadow_uniform;

layout(set = 0, binding = 6) uniform sampler2DArrayShadow shadow_sampler;

layout(constant_id = 0) const uint DIRECTIONAL_LIGHT_COUNT = 0U;
layout(constant_id = 1) const uint POINT_LIGHT_COUNT       = 0U;
layout(constant_id = 2) const uint SPOT_LIGHT_COUNT        = 0U;


#extension GL_EXT_nonuniform_qualifier : require
layout (set = 1, binding = 10 ) uniform sampler2DShadow global_textures[];

float calculate_shadow(highp vec3 pos, uint cascade_index)
{
    vec4 projected_coord = shadow_uniform.light_matrix[cascade_index] * vec4(pos, 1.0);
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
    const float bias = 0.000005;

    for(int x = -kernel_size / 2; x <= kernel_size / 2; x++) {
        for(int y = -kernel_size / 2; y <= kernel_size / 2; y++) {
            vec2 tex_offset = vec2(float(x), float(y)) * offset;
            float shadow_sample = texture(
                shadow_sampler,
                vec4(projected_coord.xy + tex_offset, cascade_index, projected_coord.z - bias)
            );
            shadow += shadow_sample;
            samples++;
        }
    }

    shadow /= float(samples);
    return shadow;
}

vec3 reinhard_extended(vec3 hdr_color, float max_white) {
    vec3 numerator = hdr_color * (1.0 + (hdr_color / (max_white * max_white)));
    return numerator / (1.0 + hdr_color);
}

void main()
{
	// Retrieve position from depth

	vec2 screen_uv = gl_FragCoord.xy / vec2(textureSize(i_depth, 0));

	float depth = texture(i_depth, screen_uv).x;
    
#ifdef USE_IBL
    if(depth < 0.000000001) { 
        vec4 clip = vec4(in_uv * 2.0 - 1.0, 0.0, 1.0);
        vec4 world_dir = global_uniform.inv_view_proj * clip;
        vec3 view_dir = normalize(world_dir.xyz / world_dir.w);
    
        vec3 background = textureLod(prefilteredMap, view_dir, 1.0).rgb;
        o_color = vec4(reinhard_extended(background, 1.0), 1.0);
        return;
    }
#endif

	vec4 clip = vec4(in_uv * 2.0 - 1.0, texture(i_depth, screen_uv).x, 1.0);

	highp vec4 world_w = global_uniform.inv_view_proj * clip;
	highp vec3 pos     = world_w.xyz / world_w.w;
	vec4 albedo_roughness = texture(i_albedo, screen_uv);
	vec3 albedo = albedo_roughness.rgb;

	// Transform from [0,1] to [-1,1]
	vec4 normal_metallic = texture(i_normal, screen_uv);
	vec3 normal      = normalize(2.0 * normal_metallic.xyz - 1.0);

    // Calculate shadow
	uint cascade_i = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i) {
		if(texture(i_depth, screen_uv).x < shadow_uniform.far_d[i]) {	
			cascade_i = i;
		}
	}

	// Calculate lighting
	vec3 L = vec3(0.0);
	float roughness = albedo_roughness.w;
    float metallic = normal_metallic.w;

    vec3 camera_pos = global_uniform.camera_pos.xyz;

	for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
	{
		L += apply_directional_light(lights_info.directional_lights[i], pos, normal, albedo, metallic, roughness, camera_pos);
		if(i==0U)
		{
			L *= calculate_shadow(pos, cascade_i);
		}
	}
	for (uint i = 0U; i < POINT_LIGHT_COUNT; ++i)
	{
		L += apply_point_light(lights_info.point_lights[i], pos, normal, albedo, metallic, roughness, camera_pos);
	}
	for (uint i = 0U; i < SPOT_LIGHT_COUNT; ++i)
	{
		L += apply_spot_light(lights_info.spot_lights[i], pos, normal, albedo, metallic, roughness, camera_pos);
	}

	vec3 emissive = texture(i_emissive, screen_uv).xyz;
	float luminance = dot(emissive, vec3(0.299, 0.587, 0.114));
	float gain = 1.0 + smoothstep(0.5, 1.0, luminance) * 150.0;
	emissive *= gain;

    vec3 final_color = L + emissive;
    vec3 ambient_color = vec3(0.2) * albedo.xyz;
#ifdef USE_IBL
	vec3 V = normalize(camera_pos - pos);
	vec3 ibl = calculate_ibl(normal, V, albedo, metallic, roughness);
	final_color += ibl * 1.0;
#else
    final_color += ambient_color;
#endif

#ifdef SHOW_CASCADE_VIEW
    vec3 cascade_overlay = vec3(0.0);
    if (cascade_i == 0) {
        cascade_overlay = vec3(0.2, 0.3, 0.6);
    } else if (cascade_i == 1) {
        cascade_overlay = vec3(0.3, 0.6, 0.3);
    } else if (cascade_i == 2) {
		cascade_overlay = vec3(0.6, 0.4, 0.2);
    } else if (cascade_i == 3) {
        cascade_overlay = vec3(0.6, 0.3, 0.6);
    }
    final_color = mix(final_color, final_color + cascade_overlay, 0.3);
#endif
	
	final_color = reinhard_extended(final_color, 1.0);

	o_color = vec4(final_color, 1.0);

}