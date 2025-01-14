#version 450

precision highp float;

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;

layout (location = 0) out vec4 o_albedo; // albede + roughness
layout (location = 1) out vec4 o_normal; // normal + metallic
layout (location = 2) out vec4 o_emissive; 

layout(set = 0, binding = 1) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
} global_uniform;

layout (set = 1, binding = 10 ) uniform sampler2D global_textures[];

layout(push_constant, std430) uniform PBRMaterialUniform {
    // x = diffuse index, y = metallic_roughness_occlusion_texture, z = normal index, w = emissive index
	uvec4       texture_indices;
    vec4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
} pbr_material_uniform;

void main(void)
{

    vec4 base_color = vec4(1.0, 0.0, 0.0, 1.0);

#ifdef HAS_BASE_COLOR_TEXTURE
    base_color = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.x)], in_uv);
#else
    base_color = pbr_material_uniform.base_color_factor;
#endif

#ifdef HAS_METALLIC_ROUGHNESS_TEXTURE
    base_color.w = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.y)], in_uv).y;
    o_normal.w = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.y)], in_uv).z;
#else
    base_color.w = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.y)], in_uv).y;
    o_normal.w = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.y)], in_uv).z;
//    base_color.w = pbr_material_uniform.roughness_factor;
//    o_normal.w = pbr_material_uniform.metallic_factor;
#endif

#ifdef HAS_EMISSIVE_TEXTURE
    o_emissive = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.w)], in_uv);
#else
    o_emissive = vec4(0.0);
#endif

    vec3 N = normalize(in_normal);
    vec3 T = normalize(in_tangent);
    vec3 B = normalize(cross(N, T)); // 在PS计算副切线
    mat3 TBN = mat3(T, B, N);

#ifdef HAS_NORMAL_TEXTURE
    vec3 normalMap = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.z)], in_uv).rgb;
    normalMap = normalMap * 2.0 - 1.0;
    N = normalize(TBN * normalMap);
#endif

    o_normal.xyz = 0.5 * N + 0.5;

    o_albedo = base_color;
}