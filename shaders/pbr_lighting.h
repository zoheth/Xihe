const float PI = 3.14159265359;

struct Light
{
	vec4 position;         // position.w represents type of light
	vec4 color;            // color.w represents light intensity
	vec4 direction;        // direction.w represents range
	vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

float D_GGX(float NoH, float roughness)
{
	float alpha       = roughness * roughness;
	float alpha2      = alpha * alpha;
	float NoH2        = NoH * NoH;
	float denominator = (NoH2 * (alpha2 - 1.0) + 1.0);
	return alpha2 / (PI * denominator * denominator);
}

float G_Smith(float NoV, float NoL, float roughness)
{
	float r   = roughness + 1.0;
	float k   = (r * r) / 8.0;
	float G1V = NoV / (NoV * (1.0 - k) + k);
	float G1L = NoL / (NoL * (1.0 - k) + k);
	return G1V * G1L;
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calculate_pbr(vec3 L, vec3 V, vec3 N, vec3 light_color, vec3 albedo, float metallic, float roughness)
{
	vec3 H = normalize(V + L);

	float NoV = max(dot(N, V), 0.001);
	float NoL = max(dot(N, L), 0.001);
	float NoH = max(dot(N, H), 0.0);
	float LoH = max(dot(L, H), 0.0);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	// Cook-Torrance BRDF
	float D = D_GGX(NoH, roughness);
	float G = G_Smith(NoV, NoL, roughness);
	vec3  F = F_Schlick(LoH, F0);

	vec3 spec    = (D * G * F) / max(4.0 * NoV * NoL, 0.001);
	vec3 kD      = (vec3(1.0) - F) * (1.0 - metallic);
	vec3 diffuse = kD * albedo / PI;

	return (diffuse + spec) * light_color * NoL;
}

vec3 apply_directional_light(Light light, vec3 pos, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 camera_pos)
{
	/* vec3 world_to_light = -light.direction.xyz;
	 world_to_light = normalize(world_to_light);
	 float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);
	 return ndotl * light.color.w * light.color.rgb * albedo;*/

	vec3 L           = normalize(-light.direction.xyz);
	vec3 V           = normalize(camera_pos - pos);
	vec3 light_color = light.color.rgb * light.color.w;

	return calculate_pbr(L, V, normal, light_color, albedo, metallic, roughness);
}

vec3 apply_point_light(Light light, vec3 pos, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 camera_pos)
{
	vec3  L    = light.position.xyz - pos;
	float dist = length(L);
	L          = normalize(L);

	float atten       = 1.0 / (dist * dist * 0.01);
	vec3  light_color = light.color.rgb * light.color.w * atten;
	vec3  V           = normalize(camera_pos - pos);

	return calculate_pbr(L, V, normal, light_color, albedo, metallic, roughness);
}

vec3 apply_spot_light(Light light, vec3 pos, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 camera_pos)
{
	vec3  L    = light.position.xyz - pos;
	float dist = length(L);
	L          = normalize(L);

	vec3  light_to_pixel = -L;
	float theta          = dot(light_to_pixel, normalize(light.direction.xyz));
	float intensity      = smoothstep(light.info.y, light.info.x, acos(theta));

	float atten       = 1.0 / (dist * dist * 0.01);
	vec3  light_color = light.color.rgb * light.color.w * intensity * atten;
	vec3  V           = normalize(camera_pos - pos);

	return calculate_pbr(L, V, normal, light_color, albedo, metallic, roughness);
}

vec3 getReflectionVector(vec3 N, vec3 V)
{
	return reflect(-V, N);
}

float calculateLODLevel(float roughness, float numMipLevels)
{
	return roughness * (numMipLevels - 1);
}

vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	float r1 = 1.0 - roughness;
	return F0 + (max(vec3(r1), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calculate_ibl(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
	vec3  F0  = mix(vec3(0.04), albedo, metallic);
	float NoV = max(dot(N, V), 0.001);

	// 计算反射向量
	vec3 R = reflect(-V, N);

	// 1. 漫反射IBL部分
	vec3 irradiance = texture(samplerIrradiance, N).rgb;
	vec3 diffuse    = irradiance * albedo;

	// 2. 镜面反射IBL部分
	// 根据粗糙度计算合适的mip level
	float lod              = roughness * 10;
	vec3  prefilteredColor = textureLod(prefilteredMap, R, lod).rgb;

	// 3. 从BRDF LUT中获取环境BRDF值
	vec2 brdf = texture(samplerBRDFLUT, vec2(NoV, roughness)).rg;

	// 计算Fresnel项
	vec3 F = F_SchlickRoughness(NoV, F0, roughness);

	// 计算镜面和漫反射比例
	vec3 kS = F;
	vec3 kD = (1.0 - kS) * (1.0 - metallic);

	// 组合漫反射和镜面反射
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	// 应用IBL强度系数
	return (kD * diffuse + specular);
}