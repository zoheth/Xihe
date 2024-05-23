#include "gltf_loader.h"

#include "common/logging.h"
#include "platform/filesystem.h"
#include "scene_graph/scene.h"

namespace xihe
{
GltfLoader::GltfLoader(backend::Device &device) :
    device_{device}
{}

std::unique_ptr<sg::Scene> GltfLoader::read_scene_from_file(const std::string &file_name, int scene_index)
{
	std::string err;
	std::string warn;

	tinygltf::TinyGLTF loader;

	std::string gltf_file_path = fs::path::get(fs::path::Type::kAssets) + file_name;

	bool import_result = loader.LoadASCIIFromFile(&model_, &err, &warn, gltf_file_path);

	if (!import_result)
	{
		LOGE("Failed to load gltf file {}.", gltf_file_path.c_str());

		return nullptr;
	}

	if (!err.empty())
	{
		LOGE("Error loading gltf model: {}.", err.c_str());

		return nullptr;
	}

	if (!warn.empty())
	{
		LOGW("{}", warn.c_str());
	}

	size_t last_slash = file_name.find_last_of('/');

	model_path_ = file_name.substr(0, last_slash);

	if (last_slash == std::string::npos)
	{
		model_path_.clear();
	}

	return std::make_unique<sg::Scene>(load_scene(scene_index));
}

sg::Scene GltfLoader::load_scene(int scene_index)
{
	
}
}        // namespace xihe
