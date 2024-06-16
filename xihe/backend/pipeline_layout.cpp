#include "pipeline_layout.h"

#include "device.h"

namespace xihe::backend
{
PipelineLayout::PipelineLayout(Device &device, const std::vector<ShaderModule *> &shader_modules, BindlessDescriptorSet *bindless_descriptor_set) :
    device_(device),
    shader_modules_(shader_modules),
	bindless_descriptor_set_{bindless_descriptor_set}
{
	aggregate_shader_resources();

	if (bindless_descriptor_set)
	{
		// Iterate over all shader sets except the last one to create descriptor set layouts.
		// The last set is intended to be used as a bindless descriptor set, hence it is skipped.
		for (auto shader_set_it = shader_sets_.begin(); shader_set_it != std::prev(shader_sets_.end()); ++shader_set_it)
		{
			descriptor_set_layouts_.emplace_back(&device_.get_resource_cache().request_descriptor_set_layout(shader_set_it->first, shader_modules, shader_set_it->second));
		}
	}
	else
	{
		// Create a descriptor set layout for each shader set in the shader modules
		for (auto &shader_set_it : shader_sets_)
		{
			descriptor_set_layouts_.emplace_back(&device_.get_resource_cache().request_descriptor_set_layout(shader_set_it.first, shader_modules, shader_set_it.second));
		}
	}

	// Collect all the descriptor set layout handles, maintaining set order
	std::vector<vk::DescriptorSetLayout> descriptor_set_layout_handles;
	for (auto descriptor_set_layout : descriptor_set_layouts_)
	{
		descriptor_set_layout_handles.push_back(descriptor_set_layout ? descriptor_set_layout->get_handle() : nullptr);
	}

	if (bindless_descriptor_set)
	{
		descriptor_set_layout_handles.push_back(bindless_descriptor_set->get_layout());
		bindless_descriptor_set_index_ = static_cast<uint32_t>(descriptor_set_layout_handles.size() - 1);
	}

	// Collect all the push constant shader resources
	std::vector<vk::PushConstantRange> push_constant_ranges;
	for (auto &push_constant_resource : get_resources(ShaderResourceType::kPushConstant))
	{
		push_constant_ranges.emplace_back(push_constant_resource.stages, push_constant_resource.offset, push_constant_resource.size);
	}

	vk::PipelineLayoutCreateInfo create_info({}, descriptor_set_layout_handles, push_constant_ranges);

	handle_ = device_.get_handle().createPipelineLayout(create_info);
}

PipelineLayout::PipelineLayout(PipelineLayout &&other) :
    device_{other.device_},
    handle_{other.handle_},
    shader_modules_{std::move(other.shader_modules_)},
    shader_resources_{std::move(other.shader_resources_)},
    shader_sets_{std::move(other.shader_sets_)},
    descriptor_set_layouts_{std::move(other.descriptor_set_layouts_)},
	bindless_descriptor_set_{other.bindless_descriptor_set_},
	bindless_descriptor_set_index_{other.bindless_descriptor_set_index_}
{
	other.handle_ = nullptr;
}

PipelineLayout::~PipelineLayout()
{
	if (handle_)
	{
		device_.get_handle().destroyPipelineLayout(handle_);
	}
}

vk::PipelineLayout PipelineLayout::get_handle() const
{
	return handle_;
}

std::vector<backend::ShaderResource> PipelineLayout::get_resources(const backend::ShaderResourceType &type, vk::ShaderStageFlags stage) const
{
	std::vector<backend::ShaderResource> found_resources;

	for (auto &it : shader_resources_)
	{
		auto &shader_resource = it.second;

		if (shader_resource.type == type || type == ShaderResourceType::kAll)
		{
			if (shader_resource.stages == stage || stage == vk::ShaderStageFlagBits::eAll)
			{
				found_resources.push_back(shader_resource);
			}
		}
	}

	return found_resources;
}

const std::vector<ShaderModule *> &PipelineLayout::get_shader_modules() const
{
	return shader_modules_;
}

vk::ShaderStageFlags PipelineLayout::get_push_constant_range_stage(uint32_t size, uint32_t offset) const
{
	vk::ShaderStageFlags stages;

	for (auto &push_constant_resource : get_resources(ShaderResourceType::kPushConstant))
	{
		if (push_constant_resource.offset <= offset && offset + size <= push_constant_resource.offset + push_constant_resource.size)
		{
			stages |= push_constant_resource.stages;
		}
	}
	return stages;
}

DescriptorSetLayout const &PipelineLayout::get_descriptor_set_layout(const uint32_t set_index) const
{
	auto it = std::ranges::find_if(descriptor_set_layouts_,
	                               [&set_index](const DescriptorSetLayout *const descriptor_set_layout) { return descriptor_set_layout->get_index() == set_index; });
	if (it == descriptor_set_layouts_.end())
	{
		throw std::runtime_error("Couldn't find descriptor set layout at set index " + to_string(set_index));
	}
	return **it;
}

const std::map<uint32_t, std::vector<ShaderResource>> &PipelineLayout::get_shader_sets() const
{
	return shader_sets_;
}

bool PipelineLayout::has_descriptor_set_layout(const uint32_t set_index) const
{
	return set_index < descriptor_set_layouts_.size();
}

vk::DescriptorSet PipelineLayout::get_bindless_descriptor_set() const
{
	if (bindless_descriptor_set_)
	{
		return bindless_descriptor_set_->get_handle();
	}
	return nullptr;
}

uint32_t PipelineLayout::get_bindless_descriptor_set_index() const
{
	return bindless_descriptor_set_index_;
}

void PipelineLayout::aggregate_shader_resources()
{
	// Collect and combine all the shader resources from all the shader modules
	// Collect them all into a map that is indexed by the name of the resource
	for (auto *shader_module : shader_modules_)
	{
		for (const auto &shader_resource : shader_module->get_resources())
		{
			std::string key = shader_resource.name;

			if (shader_resource.type == ShaderResourceType::kInput || shader_resource.type == ShaderResourceType::kOutput)
			{
				key = to_string(shader_resource.stages) + "_" + key;
			}

			auto it = shader_resources_.find(key);

			if (it != shader_resources_.end())
			{
				// Append stage flags if resource already exists
				it->second.stages |= shader_resource.stages;
			}
			else
			{
				// Create a new entry in the map
				shader_resources_.emplace(key, shader_resource);
			}
		}
	}

	// Sift through the map of name indexed shader resources
	// Separate them into their respective sets
	for (auto &it : shader_resources_)
	{
		auto &shader_resource = it.second;

		// Find binding by set index in the map.
		auto it2 = shader_sets_.find(shader_resource.set);

		if (it2 != shader_sets_.end())
		{
			// Add resource to the found set index
			it2->second.push_back(shader_resource);
		}
		else
		{
			// Create a new set index and with the first resource
			shader_sets_.emplace(shader_resource.set, std::vector<ShaderResource>{shader_resource});
		}
	}
}
}        // namespace xihe::backend
