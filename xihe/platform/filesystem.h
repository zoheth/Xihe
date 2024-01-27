#pragma once
#include <string>
#include <unordered_map>

namespace xihe::fs
{
namespace path
{

enum Type
{
	// Relative paths
	kAssets,
	kShaders,
	kStorage,
	kScreenshots,
	kLogs,
	/* NewFolder */
	kTotalRelativePathTypes,

	// Special paths
	kExternalStorage,
	kWorkingDir = kExternalStorage,
	kTemp
};

extern const std::unordered_map<Type, std::string> kRelativePaths;

const std::string get(Type type, const std::string &file = "");

}        // namespace path


bool is_directory(const std::string &path);

void create_path(const std::string &root, const std::string &path);

std::string read_text_file(const std::string &filename);

std::string read_shader(const std::string &filename);
}        // namespace xihe::fs
