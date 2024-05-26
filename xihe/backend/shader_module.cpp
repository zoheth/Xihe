#include "shader_module.h"

#include <ranges>
#include <fmt/format.h>

#include "backend/shader_compiler/glsl_compiler.h"
#include "backend/shader_compiler/spirv_reflection.h"
#include "common/error.h"
#include "common/logging.h"
#include "common/strings.h"
#include "platform/filesystem.h"

namespace xihe::backend
{

inline std::vector<std::string> precompile_shader(const std::string &source)
{
	std::vector<std::string> final_file;

	auto lines = split(source, '\n');

	for (auto &line : lines)
	{
		if (line.find("#include \"") == 0)
		{
			// Include paths are relative to the base shader directory
			std::string  include_path = line.substr(10);
			const size_t last_quote   = include_path.find('\"');
			if (!include_path.empty() && last_quote != std::string::npos)
			{
				include_path = include_path.substr(0, last_quote);
			}

			auto include_file = precompile_shader(fs::read_shader(include_path));
			for (auto &include_file_line : include_file)
			{
				final_file.push_back(include_file_line);
			}
		}
		else
		{
			final_file.push_back(line);
		}
	}

	return final_file;
}

inline std::vector<uint8_t> convert_to_bytes(std::vector<std::string> &lines)
{
	std::vector<uint8_t> bytes;
	for (auto &line : lines)
	{
		line += '\n';
		std::vector<uint8_t> line_bytes{line.begin(), line.end()};
		bytes.insert(bytes.end(), line_bytes.begin(), line_bytes.end());
	}

	return bytes;
}

ShaderVariant::ShaderVariant(std::string &&preamble, std::vector<std::string> &&processes) :
    preamble_{std::move(preamble)},
    processes_{std::move(processes)}
{
	update_id();
}

size_t ShaderVariant::get_id() const
{
	return id_;
}

void ShaderVariant::add_definitions(const std::vector<std::string> &definitions)
{
	for (auto &definition : definitions)
	{
		add_define(definition);
	}
}

void ShaderVariant::add_define(const std::string &def)
{
	processes_.push_back("D" + def);

	std::string tmp_def = def;

	// The "=" needs to turn into a space
	size_t pos_equal = tmp_def.find_first_of("=");
	if (pos_equal != std::string::npos)
	{
		tmp_def[pos_equal] = ' ';
	}

	preamble_.append("#define " + tmp_def + "\n");

	update_id();
}

void ShaderVariant::add_undefine(const std::string &undef)
{
	processes_.push_back("U" + undef);

	preamble_.append("#undef " + undef + "\n");

	update_id();
}

void ShaderVariant::add_runtime_array_size(const std::string &runtime_array_name, size_t size)
{
	if (!runtime_array_sizes_.contains(runtime_array_name))
	{
		runtime_array_sizes_.insert({runtime_array_name, size});
	}
	else
	{
		runtime_array_sizes_[runtime_array_name] = size;
	}
}

void ShaderVariant::set_runtime_array_sizes(const std::unordered_map<std::string, size_t> &sizes)
{
	runtime_array_sizes_ = sizes;
}

const std::string & ShaderVariant::get_preamble() const
{
	return preamble_;
}

const std::vector<std::string> & ShaderVariant::get_processes() const
{
	return processes_;
}

const std::unordered_map<std::string, size_t> & ShaderVariant::get_runtime_array_sizes() const
{
	return runtime_array_sizes_;
}

void ShaderVariant::clear()
{
	preamble_.clear();
	processes_.clear();
	runtime_array_sizes_.clear();
	update_id();
}

void ShaderVariant::update_id()
{
	constexpr std::hash<std::string> hasher{};
	id_ = hasher(preamble_);
}

ShaderSource::ShaderSource(const std::string &filename) :
    filename_{filename},
    source_{fs::read_shader(filename)}
{
	constexpr std::hash<std::string> hasher{};
	id_ = hasher(source_);
}

size_t ShaderSource::get_id() const
{
	return id_;
}

std::string ShaderSource::get_filename() const
{
	return filename_;
}

const std::string &ShaderSource::get_source() const
{
	return source_;
}

ShaderModule::ShaderModule(Device &device, vk::ShaderStageFlagBits stage, const ShaderSource &glsl_source, const std::string &entry_point, const ShaderVariant &shader_variant) :
    device_{device},
    stage_{stage},
    entry_point_{entry_point}
{
	debug_name_ = fmt::format("{} [variant {:X}] [entrypoint {}]",
	                          glsl_source.get_filename(),
	                          shader_variant.get_id(),
	                          entry_point);

	if (entry_point.empty())
	{
		throw VulkanException{vk::Result::eErrorInitializationFailed};
	}

	auto &source = glsl_source.get_source();

	if (source.empty())
	{
		throw VulkanException{vk::Result::eErrorInitializationFailed};
	}

	auto glsl_final_source = precompile_shader(source);

	GlslCompiler glsl_compiler;

	if (!glsl_compiler.compile_to_spirv(stage, convert_to_bytes(glsl_final_source), entry_point, shader_variant, spirv_, info_log_))
	{
		LOGE("Shader compilation failed for shader \"{}\"", glsl_source.get_filename());
		LOGE("{}", info_log_);
		throw VulkanException{vk::Result::eErrorInitializationFailed};
	}

	if (!SpirvReflection::reflect_shader_resources(stage_, spirv_, resources_, shader_variant))
	{
		LOGE("Shader reflection failed for shader \"{}\"", glsl_source.get_filename());
		throw VulkanException{vk::Result::eErrorInitializationFailed};
	}

	constexpr std::hash<std::string> hasher{};
	id_ = hasher(std::string{reinterpret_cast<const char *>(spirv_.data()),
	                        reinterpret_cast<const char *>(spirv_.data() + spirv_.size())});
}

ShaderModule::ShaderModule(ShaderModule &&other) noexcept :
	device_{other.device_},
	id_{other.id_},
	stage_{other.stage_},
	entry_point_{std::move(other.entry_point_)},
	debug_name_{std::move(other.debug_name_)},
	spirv_{std::move(other.spirv_)},
	resources_{std::move(other.resources_)},
	info_log_{std::move(other.info_log_)}
{
	other.stage_ = {};
}

size_t ShaderModule::get_id() const
{
	return id_;
}

vk::ShaderStageFlagBits ShaderModule::get_stage() const
{
	return stage_;
}

const std::string & ShaderModule::get_entry_point() const
{
	return entry_point_;
}

const std::vector<ShaderResource> & ShaderModule::get_resources() const
{
	return resources_;
}

const std::string & ShaderModule::get_info_log() const
{
	return info_log_;
}

const std::vector<uint32_t> & ShaderModule::get_binary() const
{
	return spirv_;
}

void ShaderModule::set_resource_mode(const std::string &name, const ShaderResourceMode &resource_mode)
{
	const auto it = std::ranges::find_if(resources_, [&name](const ShaderResource &resource) {
		return resource.name == name;
	});

	if (it != resources_.end())
	{
		if (resource_mode == ShaderResourceMode::kDynamic)
		{
			if (it->type == ShaderResourceType::kBufferUniform || it->type ==ShaderResourceType::kBufferStorage)
			{
				it->mode = resource_mode;
			}
			else
			{
				LOGW("Cannot set resource mode to dynamic for resource \"{}\" in shader \"{}\". Only uniform and storage buffers can be dynamic.", name, debug_name_);
			}
		}
		else
		{
			it->mode = resource_mode;
		}
	}
	else
	{
		LOGW("Cannot set resource mode for resource \"{}\" in shader \"{}\". Resource not found.", name, debug_name_);
	}
}
}        // namespace xihe::backend
