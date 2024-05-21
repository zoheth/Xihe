#include "descriptor_set_layout.h"

#include "backend/device.h"
#include "backend/physical_device.h"
#include "backend/shader_module.h"
#include "common/helpers.h"
#include "common/logging.h"

namespace xihe::backend
{
namespace
{
vk::DescriptorType find_descriptor_type(ShaderResourceType resource_type, bool dynamic)
{
	switch (resource_type)
	{
		case ShaderResourceType::kInputAttachment:
			return vk::DescriptorType::eInputAttachment;
		case ShaderResourceType::kImage:
			return dynamic ? vk::DescriptorType::eStorageImage : vk::DescriptorType::eSampledImage;
		case ShaderResourceType::kImageSampler:
			return vk::DescriptorType::eCombinedImageSampler;
		case ShaderResourceType::kImageStorage:
			return vk::DescriptorType::eStorageImage;
		case ShaderResourceType::kSampler:
			return vk::DescriptorType::eSampler;
		case ShaderResourceType::kBufferUniform:
			return dynamic ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
		case ShaderResourceType::kBufferStorage:
			return dynamic ? vk::DescriptorType::eStorageBufferDynamic : vk::DescriptorType::eStorageBuffer;
		default:
			throw std::runtime_error("No conversion possible for the shader resource type.");
	}
}

bool validate_binding(const vk::DescriptorSetLayoutBinding &binding, const std::vector<vk::DescriptorType> &blacklist)
{
	return std::ranges::find_if(blacklist, [binding](const vk::DescriptorType &type) { return type == binding.descriptorType; }) == blacklist.end();
}

bool validate_flags(const PhysicalDevice &gpu, const std::vector<vk::DescriptorSetLayoutBinding> &bindings, const std::vector<vk::DescriptorBindingFlagsEXT> &flags)
{
	// Assume bindings are valid if there are no flags
	if (flags.empty())
	{
		return true;
	}

	// Binding count has to equal flag count as its a 1:1 mapping
	if (bindings.size() != flags.size())
	{
		LOGE("Binding count has to be equal to flag count.");
		return false;
	}

	return true;
}

}        // namespace

DescriptorSetLayout::DescriptorSetLayout(Device &device, const uint32_t set_index, const std::vector<ShaderModule *> &shader_modules, const std::vector<ShaderResource> &resource_set) :
    device_{device},
    set_index_{set_index},
    shader_modules_{shader_modules}
{
	// NOTE: `shader_modules` is passed in mainly for hashing their handles in `request_resource`.
	//        This way, different pipelines (with different shaders / shader variants) will get
	//        different descriptor set layouts (incl. appropriate name -> binding lookups)

	for (auto &resource : resource_set)
	{
		// Skip shader resource whitout a binding point
		if (resource.type == ShaderResourceType::kInput ||
		    resource.type == ShaderResourceType::kOutput ||
		    resource.type == ShaderResourceType::kPushConstant ||
		    resource.type == ShaderResourceType::kSpecializationConstant)
		{
			continue;
		}

		auto descriptor_type = find_descriptor_type(resource.type, resource.mode == ShaderResourceMode::kDynamic);

		if (resource.mode == ShaderResourceMode::kUpdateAfterBind)
		{
			binding_flags_.emplace_back(vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind);
		}
		else
		{
			// When creating a descriptor set layout, if we give a structure to create_info.pNext, each binding needs to have a binding flag
			// (pBindings[i] uses the flags in pBindingFlags[i])
			// Adding {} ensures the bindings that dont use any flags are mapped correctly.
			binding_flags_.emplace_back();
		}

		vk::DescriptorSetLayoutBinding layout_binding{};

		layout_binding.binding         = resource.binding;
		layout_binding.descriptorCount = resource.array_size;
		layout_binding.descriptorType  = descriptor_type;
		layout_binding.stageFlags      = resource.stages;

		bindings_.push_back(layout_binding);

		bindings_lookup_.emplace(resource.binding, layout_binding);

		binding_flags_lookup_.emplace(resource.binding, binding_flags_.back());

		resources_lookup_.emplace(resource.name, resource.binding);
	}

	vk::DescriptorSetLayoutCreateInfo create_info{
	    {}, bindings_};

	vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags_create_info;
	if (std::ranges::find_if(resource_set, [](const ShaderResource &shader_resource) {
		    return shader_resource.mode == ShaderResourceMode::kUpdateAfterBind;
	    }) != resource_set.end())
	{
		// Spec states you can't have ANY dynamic resources if you have one of the bindings set to update-after-bind
		if (std::ranges::find_if(resource_set,
		                         [](const ShaderResource &shader_resource) { return shader_resource.mode == ShaderResourceMode::kDynamic; }) != resource_set.end())
		{
			throw std::runtime_error("Cannot create descriptor set layout, dynamic resources are not allowed if at least one resource is update-after-bind.");
		}

		if (!validate_flags(device_.get_gpu(), bindings_, binding_flags_))
		{
			throw std::runtime_error("Invalid binding, couldn't create descriptor set layout.");
		}

		binding_flags_create_info.bindingCount  = to_u32(binding_flags_.size());
		binding_flags_create_info.pBindingFlags = binding_flags_.data();

		create_info.pNext = &binding_flags_create_info;
		if (std::ranges::find(binding_flags_, vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind) != binding_flags_.end())
		{
			create_info.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPoolEXT;
		}
	}

	vk::Result result = device_.get_handle().createDescriptorSetLayout(&create_info, nullptr, &handle_);

	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Cannot create DescriptorSetLayout"};
	}
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) :
    device_{other.device_},
    shader_modules_{other.shader_modules_},
    handle_{other.handle_},
    set_index_{other.set_index_},
    bindings_{std::move(other.bindings_)},
    binding_flags_{std::move(other.binding_flags_)},
    bindings_lookup_{std::move(other.bindings_lookup_)},
    binding_flags_lookup_{std::move(other.binding_flags_lookup_)},
    resources_lookup_{std::move(other.resources_lookup_)}
{
	other.handle_ = VK_NULL_HANDLE;
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		device_.get_handle().destroyDescriptorSetLayout(handle_);
	}
}

vk::DescriptorSetLayout DescriptorSetLayout::get_handle() const
{
	return handle_;
}

uint32_t DescriptorSetLayout::get_index() const
{
	return set_index_;
}

const std::vector<vk::DescriptorSetLayoutBinding> &DescriptorSetLayout::get_bindings() const
{
	return bindings_;
}

std::unique_ptr<vk::DescriptorSetLayoutBinding> DescriptorSetLayout::get_layout_binding(const uint32_t binding_index) const
{
	auto it = bindings_lookup_.find(binding_index);

	if (it == bindings_lookup_.end())
	{
		return nullptr;
	}

	return std::make_unique<vk::DescriptorSetLayoutBinding>(it->second);
}

std::unique_ptr<vk::DescriptorSetLayoutBinding> DescriptorSetLayout::get_layout_binding(const std::string &name) const
{
	auto it = resources_lookup_.find(name);

	if (it == resources_lookup_.end())
	{
		return nullptr;
	}

	return get_layout_binding(it->second);
}

const std::vector<vk::DescriptorBindingFlagsEXT> &DescriptorSetLayout::get_binding_flags() const
{
	return binding_flags_;
}

vk::DescriptorBindingFlagsEXT DescriptorSetLayout::get_layout_binding_flag(const uint32_t binding_index) const
{
	auto it = binding_flags_lookup_.find(binding_index);

	if (it == binding_flags_lookup_.end())
	{
		return {};
	}

	return it->second;
}

const std::vector<ShaderModule *> &DescriptorSetLayout::get_shader_modules() const
{
	return shader_modules_;
}
}        // namespace xihe::backend
