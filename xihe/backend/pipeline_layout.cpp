#include "pipeline_layout.h"

#include "device.h"

namespace xihe::backend
{
PipelineLayout::PipelineLayout(Device &device, const std::vector<ShaderModule *> &shader_modules) :
device_(device),
shader_modules_(shader_modules)
{
	// Collect and combine all the shader resources from all the shader modules
	// Collect them all into a map that is indexed by the name of the resource
	for (auto *shader_module : shader_modules)
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

	// Create a descriptor set layout for each shader set in the shader modules
	for (auto &shader_set_it : shader_sets_)
	{
		descriptor_set_layouts_.emplace_back(&device_.get_resource_cache().re);
	}
}

PipelineLayout::PipelineLayout(PipelineLayout &&other)
{}

PipelineLayout::~PipelineLayout()
{}

vk::PipelineLayout PipelineLayout::get_handle() const
{}
}
