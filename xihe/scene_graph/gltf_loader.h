#pragma once

#include <memory>
#include <unordered_map>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#define KHR_LIGHTS_PUNCTUAL_EXTENSION "KHR_lights_punctual"

namespace xihe
{
namespace backend
{
class Device;
}

namespace sg
{
class Camera;
class Image;
class Light;
class Mesh;
class Node;
class PbrMaterial;
class Sampler;
class Scene;
class SubMesh;
class Texture;
}

template <class T, class Y>
struct TypeCast
{
	Y operator()(T value) const noexcept
	{
		return static_cast<Y>(value);
	}
};

class GltfLoader
{
  public:
	GltfLoader(backend::Device &device);

	std::unique_ptr<sg::Scene> read_scene_from_file(const std::string &file_name, int scene_index = -1);

private:
	sg::Scene load_scene(int scene_index = -1);

	std::unique_ptr<sg::Node> parse_node(const tinygltf::Node &gltf_node, size_t index) const;

	std::unique_ptr<sg::Camera> parse_camera(const tinygltf::Camera &gltf_camera) const;

	std::unique_ptr<sg::Mesh> parse_mesh(const tinygltf::Mesh &gltf_mesh) const;

	std::unique_ptr<sg::PbrMaterial> parse_material(const tinygltf::Material &gltf_material) const;

	std::unique_ptr<sg::Image> parse_image(tinygltf::Image &gltf_image) const;

	std::unique_ptr<sg::Sampler> parse_sampler(const tinygltf::Sampler &gltf_sampler) const;

	std::unique_ptr<sg::Texture> parse_texture(const tinygltf::Texture &gltf_texture) const;

	std::unique_ptr<sg::PbrMaterial> create_default_material();

	std::unique_ptr<sg::Sampler> create_default_sampler(int filter);

	std::unique_ptr<sg::Camera> create_default_camera();

	/**
	 * @brief Parses and returns a list of scene graph lights from the KHR_lights_punctual extension
	 */
	std::vector<std::unique_ptr<sg::Light>> parse_khr_lights_punctual();

	bool is_extension_enabled(const std::string &requested_extension);

	/**
	 * @brief Finds whether an extension exists inside a tinygltf extension map and returns the result
	 * @param tinygltf_extensions The extension map associated with a given tinygltf object
	 * @param extension The extension to check
	 * @returns A pointer to the value of the extension object, nullptr if it isn't found
	 */
	tinygltf::Value *get_extension(tinygltf::ExtensionMap &tinygltf_extensions, const std::string &extension);

	backend::Device &device_;

	tinygltf::Model model_;

	std::string model_path_;

	static std::unordered_map<std::string, bool> supported_extensions_;


};
}
