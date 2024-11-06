/* Copyright (c) 2023, Mobica Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#version 450

precision highp float;

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in PerVertexData
{
  vec3 pos;
  vec3 normal;
  vec2 uv;
} v_in;

layout (location = 0) out vec4 o_albedo;
layout (location = 1) out vec4 o_normal;

#ifdef HAS_BASE_COLOR_TEXTURE
layout (set = 1, binding = 10 ) uniform sampler2D global_textures[];
#endif

layout(push_constant, std430) uniform PBRMaterialUniform {
    // x = diffuse index, y = roughness index, z = normal index, w = occlusion index.
	// Occlusion and roughness are encoded in the same texture
	uvec4       texture_indices;
    vec4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
} pbr_material_uniform;

void main(void)

void main()
{
	vec3 normal = normalize(v_in.normal);
	// Transform normals from [-1, 1] to [0, 1]
    o_normal = vec4(0.5 * normal + 0.5, 1.0);

	vec4 base_color = vec4(1.0, 0.0, 0.0, 1.0);
#ifdef HAS_BASE_COLOR_TEXTURE
    base_color = texture(global_textures[nonuniformEXT(pbr_material_uniform.texture_indices.x)], v_in.uv);
#else
    base_color = pbr_material_uniform.base_color_factor;
#endif
    o_albedo = base_color;
}
