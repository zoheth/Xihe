#version 450

#define SHADOW_MAP_CASCADE_COUNT 3
#define NUM_BINS 16.0
#define BIN_WIDTH ( 1.0 / NUM_BINS )
#define TILE_SIZE 8
#define MAX_POINT_LIGHT_COUNT 256
#define NUM_WORDS ( ( MAX_POINT_LIGHT_COUNT + 31 ) / 32 )
#define DEBUG_MODE 0

precision highp float;

layout(binding = 0) uniform sampler2D i_depth;
layout(binding = 1) uniform sampler2D i_albedo;
layout(binding = 2) uniform sampler2D i_normal;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 3) uniform GlobalUniform
{
    mat4 inv_view_proj;
    mat4 view; 
    float near_plane;
    float far_plane;
}
global_uniform;

layout(set = 0, binding = 7) readonly buffer ZBins {
    uint bins[];
};

layout(set = 0, binding = 8) readonly buffer Tiles {
    uint tiles[];
};

layout(set = 0, binding = 9) readonly buffer LightIndices {
    uint light_indices[];
};

#include "lighting.h"

layout(set = 0, binding = 4) uniform LightsInfo
{
	Light directional_lights[MAX_LIGHT_COUNT];
	Light point_lights[MAX_POINT_LIGHT_COUNT];
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

vec3 get_debug_bin_color(int bin_index) {
    // Create distinct colors for different bins
    float hue = float(bin_index) / NUM_BINS;
    
    // Simple HSV to RGB conversion for distinct colors
    vec3 rgb;
    float h = hue * 6.0;
    float i = floor(h);
    float f = h - i;
    float p = 0.0;
    float q = 1.0 - f;
    float t = f;

    if (i == 0.0) rgb = vec3(1.0, t, p);
    else if (i == 1.0) rgb = vec3(q, 1.0, p);
    else if (i == 2.0) rgb = vec3(p, 1.0, t);
    else if (i == 3.0) rgb = vec3(p, q, 1.0);
    else if (i == 4.0) rgb = vec3(t, p, 1.0);
    else rgb = vec3(1.0, p, q);

    return rgb;
}

vec4 get_debug_visualization(vec3 pos, vec2 screen_uv, vec3 normal) {
    // Calculate view space depth and bin index
    vec4 view_pos = global_uniform.view * vec4(pos, 1.0);
    float linear_d = (view_pos.z - global_uniform.near_plane) / 
                    (global_uniform.far_plane - global_uniform.near_plane);
    int bin_index = int(linear_d / BIN_WIDTH);
    
    // Get light range for current bin
    uint bin_value = bins[bin_index];
    uint min_light_id = bin_value & 0xFFFF;
    uint max_light_id = (bin_value >> 16) & 0xFFFF;
    
    // Calculate tile coordinates
    uvec2 position = uvec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);
    uvec2 tile = position / uint(TILE_SIZE);
    
    uint screen_width = uint(textureSize(i_depth, 0).x);
    uint num_tiles_x = uint(screen_width) / uint(TILE_SIZE);
    uint stride = uint(NUM_WORDS) * num_tiles_x;
    uint tile_base = tile.y * stride + tile.x;
    
    // Count actual lights affecting this pixel
    uint light_count = 0;
    if(min_light_id != MAX_POINT_LIGHT_COUNT + 1) {
        for(uint i = min_light_id; i <= max_light_id; ++i) {
            uint word_index = i / 32;
            uint bit_index = i % 32;
            if((tiles[tile_base + word_index] & ( 1u << bit_index )) != 0) {
                light_count++;
            }
        }
    }

    switch(DEBUG_MODE) {
        case 1: // Visualize bins
            return vec4(get_debug_bin_color(bin_index), 1.0);
            
        case 2: // Visualize tiles
            bool tile_edge = (position.x % TILE_SIZE < 1) || (position.y % TILE_SIZE < 1);
            return tile_edge ? vec4(1.0, 1.0, 0.0, 1.0) : vec4(0.2, 0.2, 0.2, 1.0);
            
        case 3: // Visualize light counts
            float intensity = float(light_count) / 32.0; // Assume max 32 lights per tile
            return vec4(intensity, intensity * 0.5, 0.0, 1.0);
            
        case 4: // Visualize light index range
            if(min_light_id == MAX_POINT_LIGHT_COUNT + 1) {
                return vec4(1.0, 0.0, 0.0, 1.0); // Red for invalid
            }
            return vec4(
                float(min_light_id) / float(MAX_POINT_LIGHT_COUNT),
                float(max_light_id) / float(MAX_POINT_LIGHT_COUNT),
                float(max_light_id - min_light_id) / float(MAX_POINT_LIGHT_COUNT),
                1.0
            );
            
        default: // Normal rendering
            return vec4(0.0); // Will be ignored in main()
    }
}

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
    const float bias = 0.005;

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

//    o_color = vec4(vec3(texture(i_depth, screen_uv).x), 1.0);
//    return;

    vec4 debug_color = get_debug_visualization(pos, screen_uv, normal);
    if(DEBUG_MODE > 0) {
        o_color = debug_color;
        return;
    }

    // Calculate shadow
	uint cascade_i = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i) {
		if(texture(i_depth, screen_uv).x < shadow_uniform.far_d[i]) {	
			cascade_i = i;
		}
	}
    // Calculate directional light
	vec3 L = vec3(0.0);
	for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
	{
		L += apply_directional_light(lights_info.directional_lights[i], normal);
		if(i==0U)
		{
			L *= calculate_shadow(pos, cascade_i);
		}
	}
    
    // Calculate view space depth and bin index
    vec4 view_pos = global_uniform.view * vec4(pos, 1.0);
    float linear_d = (-view_pos.z - global_uniform.near_plane) / 
                    (global_uniform.far_plane - global_uniform.near_plane);
    int bin_index = int(linear_d / BIN_WIDTH);
    
    uint bin_value = bins[bin_index];
    uint min_light_id = bin_value & 0xFFFF;
    uint max_light_id = (bin_value >> 16) & 0xFFFF;

    // Calculate tile coordinates
    uvec2 position = uvec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);
    // position.y = textureSize(i_depth, 0).y - position.y;
    uvec2 tile = position / uint(TILE_SIZE);

    uint screen_width = uint(textureSize(i_depth, 0).x);
    uint num_tiles_x = uint(screen_width) / uint(TILE_SIZE);
    uint stride = uint(NUM_WORDS) * num_tiles_x;
    uint tile_base = tile.y * stride + tile.x;

    // Point Light
    if(min_light_id != MAX_POINT_LIGHT_COUNT + 1) {
	    for(uint i = min_light_id; i <= max_light_id; ++i) {
            uint word_index = i / 32;
            uint bit_index = i % 32;
            
            if((tiles[tile_base + word_index] & ( 1u << bit_index ) ) != 0) {
                uint global_light_index = light_indices[i];
                L += apply_point_light(lights_info.point_lights[global_light_index], pos, normal);
            }

//            uint global_light_index = light_indices[i];
//            L += apply_point_light(lights_info.point_lights[global_light_index], pos, normal);

        }
    }
	
	for (uint i = 0U; i < SPOT_LIGHT_COUNT; ++i)
	{
		L += apply_spot_light(lights_info.spot_lights[i], pos, normal);
	}
	vec3 ambient_color = vec3(0.05) * albedo.xyz;

	vec3 final_color = ambient_color + L * albedo.xyz;

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
	
	o_color = vec4(final_color, 1.0);
}