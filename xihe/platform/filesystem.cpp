#include "filesystem.h"

#include <filesystem>
#include <cassert>
#include <fstream>
#include <vector>

#include "platform/platform.h"

namespace xihe::fs
{
namespace path
{
const std::unordered_map<Type, std::string> kRelativePaths = {
    {Type::kAssets, "assets/"},
    {Type::kShaders, "shaders/"},
    {Type::kStorage, "output/"},
    {Type::kScreenshots, "output/images/"},
    {Type::kLogs, "output/logs/"},
};
}

const std::string path::get(Type type, const std::string &file)
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

	auto path = Platform::get_working_directory() + it->second;

	if (!is_directory(path))
	{
		create_path(Platform::get_working_directory(), it->second);
	}

	return path + file;
}


bool is_directory(const std::string &path)
{
	return std::filesystem::is_directory(path);
}

void create_path(const std::string &root, const std::string &path)
{
	std::filesystem::create_directories(std::filesystem::path(root) / std::filesystem::path(path));
}

std::string read_text_file(const std::string &filename)
{
	std::vector<std::string> data;

	std::ifstream file;

	file.open(filename, std::ios::in);

	if (!file.is_open())
	{
		throw std::runtime_error{"Failed to open file: " + filename};
	}

	return std::string{
	    (std::istreambuf_iterator<char>(file)),
	    (std::istreambuf_iterator<char>())};
}

std::string read_shader(const std::string &filename)
{
	return read_text_file(path::get(path::Type::kShaders) + filename);
}
}        // namespace xihe::fs
