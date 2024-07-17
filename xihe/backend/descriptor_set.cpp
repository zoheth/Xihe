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
	std::vector<size_t>                 write_operation_hashes;

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
		auto  binding_index     = binding_it.first;
		auto &binding_resources = binding_it.second;

		if (auto binding_info = descriptor_set_layout_.get_layout_binding(binding_index))
		{
			for (auto &element_it : binding_resources)
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

BindlessDescriptorSet::BindlessDescriptorSet(Device &device) :
    device_{device}
{
	std::vector<vk::DescriptorPoolSize> pool_sizes_bindless =
	    {
	        {vk::DescriptorType::eCombinedImageSampler,
	         max_bindless_resources_},
	        {vk::DescriptorType::eStorageImage,
	         max_bindless_resources_}

	    };
	auto pool_size_count = to_u32(pool_sizes_bindless.size());

	vk::DescriptorPoolCreateInfo pool_create_info{};

	pool_create_info.flags         = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
	pool_create_info.maxSets       = max_bindless_resources_ * pool_size_count;
	pool_create_info.poolSizeCount = pool_size_count;
	pool_create_info.pPoolSizes    = pool_sizes_bindless.data();

	vk::Result result = device_.get_handle().createDescriptorPool(&pool_create_info, nullptr, &descriptor_pool_);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error{"Failed to create bindless descriptor pool"};
	}

	std::vector<vk::DescriptorSetLayoutBinding> bindings;

	bindings.emplace_back(bindless_texture_binding_,
	                      vk::DescriptorType::eCombinedImageSampler,
	                      max_bindless_resources_,
	                      vk::ShaderStageFlagBits::eAll,
	                      nullptr);

	bindings.emplace_back(bindless_texture_binding_ + 1,
	                      vk::DescriptorType::eStorageImage,
	                      max_bindless_resources_,
	                      vk::ShaderStageFlagBits::eAll,
	                      nullptr);

	vk::DescriptorSetLayoutCreateInfo layout_create_info{
	    vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
	    bindings};

	std::vector<vk::DescriptorBindingFlags> binding_flags = {
	    vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound,
	    vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound};

	vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT extended_info{
	    binding_flags};

	layout_create_info.pNext = &extended_info;

	result = device_.get_handle().createDescriptorSetLayout(&layout_create_info, nullptr, &bindless_descriptor_set_layout_);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error{"Failed to create bindless descriptor set layout"};
	}

	vk::DescriptorSetAllocateInfo allocate_info{};
	allocate_info.descriptorPool     = descriptor_pool_;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts        = &bindless_descriptor_set_layout_;

	vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_count_info{
	    max_bindless_resources_ - 1};

	result = device_.get_handle().allocateDescriptorSets(&allocate_info, &handle_);
	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error{"Failed to allocate bindless descriptor set"};
	}
}

BindlessDescriptorSet::~BindlessDescriptorSet()
{
	device_.get_handle().destroyDescriptorSetLayout(bindless_descriptor_set_layout_);
	device_.get_handle().destroyDescriptorPool(descriptor_pool_);
}

vk::DescriptorSet BindlessDescriptorSet::get_handle() const
{
	return handle_;
}

vk::DescriptorSetLayout BindlessDescriptorSet::get_layout() const
{
	return bindless_descriptor_set_layout_;
}

void BindlessDescriptorSet::update(uint32_t index, vk::DescriptorImageInfo image_info)
{
	assert(index == next_image_index_);
	next_image_index_++;
	vk::WriteDescriptorSet write_descriptor_set{
	    handle_,
	    bindless_texture_binding_,
	    index,
	    1,
	    vk::DescriptorType::eCombinedImageSampler,
	    &image_info,
	    nullptr};

	device_.get_handle().updateDescriptorSets({write_descriptor_set}, {});
}

uint32_t BindlessDescriptorSet::update(vk::DescriptorImageInfo image_info)
{
	assert(next_image_index_ < max_bindless_resources_);
	const auto index = next_image_index_;
	update(index, image_info);
	return index;
}

uint32_t BindlessDescriptorSet::update(vk::DescriptorBufferInfo buffer_info)
{
	assert(next_buffer_index_ < max_bindless_resources_);
	const auto index = next_buffer_index_;
	next_buffer_index_++;
	vk::WriteDescriptorSet write_descriptor_set{
	    handle_,
	    bindless_buffer_binding_,
	    index,
	    1,
	    vk::DescriptorType::eStorageBuffer,
	    nullptr,
	    &buffer_info};

	device_.get_handle().updateDescriptorSets({write_descriptor_set}, {});
	return index;
}

void BindlessDescriptorSet::reset_index()
{
	next_image_index_ = 0;
}
}        // namespace xihe::backend
