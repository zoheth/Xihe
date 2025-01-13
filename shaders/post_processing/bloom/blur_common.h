#ifndef BLUR_COMMON_H_
#define BLUR_COMMON_H_

layout(set = 0, binding = 0) uniform sampler2D in_tex;
layout(rgba16f, set = 0, binding = 1) writeonly uniform image2D out_tex;

layout(set = 0, binding = 2) uniform CommonUniforms {
	uvec2 resolution;
	vec2 inv_resolution;
	vec2 inv_input_resolution;
} common_uniform;

vec2 get_uv(vec2 uv, float x, float y, float scale)
{
	return uv + common_uniform.inv_input_resolution * (vec2(x, y) * scale);
}

vec3 bloom_blur(vec2 uv, float uv_scale)
{
	vec3        rgb = vec3(0.0);
	const float N   = -1.0;
	const float Z   = 0.0;
	const float P   = 1.0;
	rgb += 0.25 * textureLod(in_tex, get_uv(uv, Z, Z, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, N, P, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, P, P, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, N, N, uv_scale), 0.0).rgb;
	rgb += 0.0625 * textureLod(in_tex, get_uv(uv, P, N, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, N, Z, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, P, Z, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, Z, N, uv_scale), 0.0).rgb;
	rgb += 0.125 * textureLod(in_tex, get_uv(uv, Z, P, uv_scale), 0.0).rgb;
	return rgb;
}

#endif