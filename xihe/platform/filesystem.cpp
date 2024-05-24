#include "filesystem.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

#include "platform/platform.h"

namespace xihe::fs
{
namespace path
{
const std::unordered_map<Type, Path> kRelativePaths = {
    {Type::kAssets, "assets"},
    {Type::kShaders, "shaders"},
    {Type::kStorage, "output/"},
    {Type::kScreenshots, "output/images"},
    {Type::kLogs, "output/logs"},
};
}

Path path::get(Type type, const Path &file)
{
	assert(kRelativePaths.size() == Type::kTotalRelativePathTypes && "Not all paths are defined in filesystem, please check that each enum is specified");

	// Check for special cases first
	if (type == Type::kWorkingDir)
	{
		return Platform::get_working_directory();
	}
	else if (type == Type::kTemp)
	{
		return Platform::get_temp_directory();
	}

	// Check for relative paths
	auto it = kRelativePaths.find(type);

	if (kRelativePaths.size() < Type::kTotalRelativePathTypes)
	{
		throw std::runtime_error("Platform hasn't initialized the paths correctly");
	}
	else if (it == kRelativePaths.end())
	{
		throw std::runtime_error("Path enum doesn't exist, or wasn't specified in the path map");
	}
	else if (it->second.empty())
	{
		throw std::runtime_error("Path was found, but it is empty");
	}

	auto path = Platform::get_working_directory() / it->second;

	if (!std::filesystem::is_directory(path))
	{
		create_directories(path);
	}

	return path / file;
}

std::string read_text_file(const Path &path)
{
	std::vector<std::string> data;

	std::ifstream file;

	file.open(path, std::ios::in);

	if (!file.is_open())
	{
		throw std::runtime_error{"Failed to open file: " + path.string()};
	}

	return std::string{
	    (std::istreambuf_iterator<char>(file)),
	    (std::istreambuf_iterator<char>())};
}

std::string read_shader(const Path &path)
{
	return read_text_file(path::get(path::Type::kShaders) / path);
}

std::vector<uint8_t> read_asset(const Path &path)
{
	return read_binary_file(path::get(path::Type::kAssets) / path);
}

std::string get_extension(const Path &path)
{
	auto extension = path.extension();

	if (extension.empty())
	{
		throw std::runtime_error{"File has no extension: " + path.string()};
	}

	return extension.string().substr(1);
}

std::vector<uint8_t> read_binary_file(const Path &path)
{
	std::ifstream file;

	file.open(path, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		throw std::runtime_error{"Failed to open file: " + path.string()};
	}

	auto size = file.tellg();

	std::vector<uint8_t> data(size);

	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char *>(data.data()), size);

	return data;
}
}        // namespace xihe::fs
