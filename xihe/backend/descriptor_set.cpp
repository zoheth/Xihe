#include "descriptor_set.h"

#include "descriptor_pool.h"
#include "descriptor_set_layout.h"
#include "device.h"
#include "resources_management/resource_caching.h"

namespace xihe::backend
{
DescriptorSet::DescriptorSet(Device &device, const DescriptorSetLayout &descriptor_set_layout, DescriptorPool &descriptor_pool, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos) :
    device_{device},
    descriptor_set_layout_{descriptor_set_layout},
    descriptor_pool_{descriptor_pool},
    buffer_infos_{buffer_infos},
    image_infos_{image_infos},
    handle_{descriptor_pool.allocate()}
{
	prepare();
}

void DescriptorSet::update(const std::vector<uint32_t> &bindings_to_update)
{
	std::vector<vk::WriteDescriptorSet> write_operations;
	std::vector<size_t>               write_operation_hashes;

	// If the 'bindings_to_update' vector is empty, we want to write to all the bindings
	// (but skipping all to-update bindings that haven't been written yet)
	if (bindings_to_update.empty())
	{
		for (size_t i = 0; i < write_descriptor_sets_.size(); i++)
		{
			const auto &write_operation = write_descriptor_sets_[i];

			size_t write_operation_hash = 0;
			hash_param(write_operation_hash, write_operation);

			auto update_pair_it = updated_bindings_.find(write_operation.dstBinding);
			if (update_pair_it == updated_bindings_.end() || update_pair_it->second != write_operation_hash)
			{
				write_operations.push_back(write_operation);
				write_operation_hashes.push_back(write_operation_hash);
			}
		}
	}
	else
	{
		// Otherwise we want to update the binding indices present in the 'bindings_to_update' vector.
		// (again, skipping those to update but not updated yet)
		for (size_t i = 0; i < write_descriptor_sets_.size(); i++)
		{
			const auto &write_operation = write_descriptor_sets_[i];

			if (std::ranges::find(bindings_to_update, write_operation.dstBinding) != bindings_to_update.end())
			{
				size_t write_operation_hash = 0;
				hash_param(write_operation_hash, write_operation);

				auto update_pair_it = updated_bindings_.find(write_operation.dstBinding);
				if (update_pair_it == updated_bindings_.end() || update_pair_it->second != write_operation_hash)
				{
					write_operations.push_back(write_operation);
					write_operation_hashes.push_back(write_operation_hash);
				}
			}
		}
	}

	// Perform the Vulkan call to update the DescriptorSet by executing the write operations
	if (!write_operations.empty())
	{
		device_.get_handle().updateDescriptorSets(write_operations, {});
	}

	// Store the bindings from the write operations that were executed by vkUpdateDescriptorSets (and their hash)
	// to prevent overwriting by future calls to "update()"
	for (size_t i = 0; i < write_operations.size(); i++)
	{
		updated_bindings_[write_operations[i].dstBinding] = write_operation_hashes[i];
	}
}

void DescriptorSet::apply_writes() const
{
	device_.get_handle().updateDescriptorSets(write_descriptor_sets_, {});
}

const DescriptorSetLayout &DescriptorSet::get_layout() const
{
	return descriptor_set_layout_;
}

vk::DescriptorSet DescriptorSet::get_handle() const
{
	return handle_;
}

void DescriptorSet::prepare()
{
	if (!write_descriptor_sets_.empty())
	{
		LOGW("Trying to prepare a descriptor set that has already been prepared, skipping.");
		return;
	}

	// Iterate over all buffer bindings
	for (auto &binding_it : buffer_infos_)
	{
		auto  binding_index   = binding_it.first;
		auto &buffer_bindings = binding_it.second;

		if (auto binding_info = descriptor_set_layout_.get_layout_binding(binding_index))
		{
			for (auto &element_it : buffer_bindings)
			{
				auto &buffer_info = element_it.second;

				size_t uniform_buffer_range_limit = device_.get_gpu().get_properties().limits.maxUniformBufferRange;
				size_t storage_buffer_range_limit = device_.get_gpu().get_properties().limits.maxStorageBufferRange;

				size_t buffer_range_limit = buffer_info.range;

				if ((binding_info->descriptorType == vk::DescriptorType::eUniformBuffer ||
				     binding_info->descriptorType == vk::DescriptorType::eUniformBufferDynamic) &&
				    buffer_range_limit > uniform_buffer_range_limit)
				{
					LOGE("Set {} binding {} cannot be updated: buffer size {} exceeds the uniform buffer range limit {}", descriptor_set_layout_.get_index(), binding_index, buffer_info.range, uniform_buffer_range_limit);
					buffer_range_limit = uniform_buffer_range_limit;
				}
				else if ((binding_info->descriptorType == vk::DescriptorType::eStorageBuffer ||
				          binding_info->descriptorType == vk::DescriptorType::eStorageBufferDynamic) &&
				         buffer_range_limit > storage_buffer_range_limit)
				{
					LOGE("Set {} binding {} cannot be updated: buffer size {} exceeds the storage buffer range limit {}", descriptor_set_layout_.get_index(), binding_index, buffer_info.range, storage_buffer_range_limit);
					buffer_range_limit = storage_buffer_range_limit;
				}

				// Clip the buffers range to the limit if one exists as otherwise we will receive a Vulkan validation error
				buffer_info.range = buffer_range_limit;

				vk::WriteDescriptorSet write_descriptor_set{
				    handle_,
				    binding_index,
				    element_it.first,
				    1,
				    binding_info->descriptorType,
				    {},
				    &buffer_info};

				write_descriptor_sets_.push_back(write_descriptor_set);
			}
		}
		else
		{
			LOGE("Shader layout set does not use buffer binding at #{}", binding_index);
		}
	}

	// Iterate over all image bindings
	for (auto &binding_it : image_infos_)
	{
		auto binding_index = binding_it.first;
		auto &binding_resources = binding_it.second;

		if (auto binding_info = descriptor_set_layout_.get_layout_binding(binding_index))
		{
			for (auto &element_it :binding_resources)
			{
				auto &image_info = element_it.second;

				vk::WriteDescriptorSet write_descriptor_set{
				    handle_,
				    binding_index,
				    element_it.first,
				    1,
				    binding_info->descriptorType,
				    &image_info,
				    {}};

				write_descriptor_sets_.push_back(write_descriptor_set);
			}
		}
		else
		{
			LOGE("Shader layout set does not use image binding at #{}", binding_index);
		}
	}
}
}        // namespace xihe::backend
