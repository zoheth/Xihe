#pragma once
#include <string>
#include <filesystem>
#include <unordered_map>

namespace xihe::fs
{
using Path = std::filesystem::path;

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

extern const std::unordered_map<Type, Path> kRelativePaths;

Path get(Type type, const Path &file = "");

}        // namespace path

std::string read_text_file(const Path &path);

std::vector<uint8_t> read_binary_file(const Path &path);

std::string read_shader(const Path &path);

std::vector<uint8_t> read_asset(const Path &path);

std::string get_extension(const Path &path);
}        // namespace xihe::fs
