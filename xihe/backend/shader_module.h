#pragma once

#include "vulkan/vulkan.hpp"

#include <unordered_map>

namespace xihe::backend
{
class Device;

enum class ShaderResourceType
{
	kInput,
	kInputAttachment,
	kOutput,
	kImage,
	kImageSampler,
	kImageStorage,
	kSampler,
	kBufferUniform,
	kBufferStorage,
	kPushConstant,
	kSpecializationConstant,
	kAll
};

/// This determines the type and method of how descriptor set should be created and bound
enum class ShaderResourceMode
{
	kStatic,
	kDynamic,
	kUpdateAfterBind
};

/// A bitmask of qualifiers applied to a resource
struct ShaderResourceQualifiers
{
	enum : uint32_t
	{
		kNone        = 0,
		kNonReadable = 1 << 0,
		kNonWritable = 1 << 1,
	};
};

/// Store shader resource data.
/// Used by the shader module.
struct ShaderResource
{
	vk::ShaderStageFlags stages;

	ShaderResourceType type;
	ShaderResourceMode mode;

	uint32_t set;
	uint32_t binding;
	uint32_t location;
	uint32_t input_attachment_index;
	uint32_t vec_size;
	uint32_t columns;
	uint32_t array_size;
	uint32_t offset;
	uint32_t size;
	uint32_t constant_id;
	uint32_t qualifiers;

	std::string name;
};

class ShaderVariant
{
  public:
	ShaderVariant() = default;

	ShaderVariant(std::string &&preamble, std::vector<std::string> &&processes);

	size_t get_id() const;

	void add_definitions(const std::vector<std::string> &definitions);

	void add_define(const std::string &def);

	void add_undefine(const std::string &undef);

	void add_runtime_array_size(const std::string &runtime_array_name, size_t size);

	void set_runtime_array_sizes(const std::unordered_map<std::string, size_t> &sizes);

	const std::string                             &get_preamble() const;
	const std::vector<std::string>                &get_processes() const;
	const std::unordered_map<std::string, size_t> &get_runtime_array_sizes() const;

	void clear();

  private:
	size_t id_{};

	std::string                             preamble_{};
	std::vector<std::string>                processes_;
	std::unordered_map<std::string, size_t> runtime_array_sizes_;

	void update_id();
};

class ShaderSource
{
  public:
	ShaderSource(const std::string &filename);

	size_t             get_id() const;
	std::string        get_filename() const;
	const std::string &get_source() const;

  private:
	size_t      id_;
	std::string filename_;
	std::string source_;
};

class ShaderModule
{
  public:
	ShaderModule(Device                 &device,
	             vk::ShaderStageFlagBits stage,
	             const ShaderSource     &glsl_source,
	             const std::string      &entry_point,
	             const ShaderVariant    &shader_variant);

	ShaderModule(const ShaderModule &) = delete;
	ShaderModule(ShaderModule &&other) noexcept;

	ShaderModule &operator=(const ShaderModule &) = delete;
	ShaderModule &operator=(ShaderModule &&)      = delete;

	size_t get_id() const;

	vk::ShaderStageFlagBits get_stage() const;

	const std::string &get_entry_point() const;

	const std::vector<ShaderResource> &get_resources() const;

	const std::string &get_info_log() const;

	const std::vector<uint32_t> &get_binary() const;

	const std::string &get_debug_name() const
	{
		return debug_name_;
	}

	void set_debug_name(const std::string &name)
	{
		debug_name_ = name;
	}

	void set_resource_mode(const std::string &name, const ShaderResourceMode &resource_mode);

  private:
	Device &device_;

	size_t id_;

	vk::ShaderStageFlagBits stage_;
	std::string             entry_point_;
	std::string             debug_name_;

	std::vector<uint32_t>       spirv_;
	std::vector<ShaderResource> resources_;

	std::string info_log_;
};
}        // namespace xihe::backend