#pragma once


#include "common/error.h"
#include "common/glm_common.h"
#include "scene_graph/component.h"
namespace xihe::sg
{
class Texture;

enum class AlphaMode
{
	/// Alpha value is ignored
	kOpaque,
	/// Either full opaque or fully transparent
	kMask,
	/// Output is combined with the background
	kBlend
};



class Material : public Component
{
  public:
	Material(const std::string &name);

	Material(Material &&other) = default;

	virtual ~Material() = default;

	virtual std::type_index get_type() override;

	std::unordered_map<std::string, Texture *> textures;

	glm::vec3 emissive{0.0f, 0.0f, 0.0f};

	bool double_sided{false};

	float alpha_cutoff{0.5f};

	AlphaMode alpha_mode{AlphaMode::kOpaque};
};

class PbrMaterial : public Material
{
  public:
	PbrMaterial(const std::string &name);

	virtual ~PbrMaterial() = default;

	virtual std::type_index get_type() override;

	void set_texture_index(const std::string &name, uint32_t texture_index);

	glm::vec4 base_color_factor{0.0f, 0.0f, 0.0f, 0.0f};

	float metallic_factor{0.0f};

	float roughness_factor{0.0f};

	// x = diffuse index, y = roughness index, z = normal index, w = occlusion index.
    // Occlusion and roughness are encoded in the same texture
	glm::uvec4 texture_indices{2, 0, 0, 0};
};
}
