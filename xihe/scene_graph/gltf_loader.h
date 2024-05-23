#pragma once

#include <memory>
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

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

class GltfLoader
{
  public:
	GltfLoader(backend::Device &device);

	std::unique_ptr<sg::Scene> read_scene_from_file(const std::string &file_name, int scene_index = -1);

private:
	sg::Scene load_scene(int scene_index = -1);


	backend::Device &device_;

	tinygltf::Model model_;

	std::string model_path_;


};
}
