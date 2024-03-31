#include "pipeline.h"

#include "device.h"

namespace xihe::backend
{
Pipeline::Pipeline(Device &device) :
    device_{device}
{}

Pipeline::Pipeline(Pipeline &&other) :
    device_{other.device_},
    handle_{other.handle_},
    state_{other.state_}
{
	other.handle_ = VK_NULL_HANDLE;
}

Pipeline::~Pipeline()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		device_.get_handle().destroyPipeline(handle_);
	}
}

VkPipeline Pipeline::get_handle() const
{
	return handle_;
}

const PipelineState &Pipeline::get_state() const
{
	return state_;
}

ComputePipeline::ComputePipeline(Device &device, vk::PipelineCache pipeline_cache, PipelineState &pipeline_state) :
    Pipeline{device}
{
	const ShaderModule *shader_module = pipeline_state.get_pipeline_layout().get_shader_modules().front();

	if (shader_module->get_stage() != vk::ShaderStageFlagBits::eCompute)
	{
		throw VulkanException{vk::Result::eErrorInvalidShaderNV, "Shader module stage is not compute"};
	}

	vk::PipelineShaderStageCreateInfo stage_create_info{
	    vk::PipelineShaderStageCreateFlags{},
	    vk::ShaderStageFlagBits::eCompute,
	    VK_NULL_HANDLE,
	    shader_module->get_entry_point().c_str(),
	    nullptr};

	vk::ShaderModuleCreateInfo create_info{
	    vk::ShaderModuleCreateFlags{},
	    shader_module->get_binary().size(),
	    reinterpret_cast<const uint32_t *>(shader_module->get_binary().data())};

	vk::Result result = device_.get_handle().createShaderModule(&create_info, nullptr, &stage_create_info.module);

	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Failed to create shader module"};
	}

	device.get_debug_utils().set_debug_name(device.get_handle(),
	                                        vk::ObjectType::eShaderModule,
	                                        reinterpret_cast<uint64_t>(static_cast<VkShaderModule>(stage_create_info.module)),
	                                        shader_module->get_debug_name().c_str());

	std::vector<uint8_t> data{};
	std::vector<vk::SpecializationMapEntry> map_entries{};

	const auto specialization_constant_stat = pipeline_state.get_specialization_constant_stat();
}

GraphicsPipeline::GraphicsPipeline(Device &device, vk::PipelineCache pipeline_cache, PipelineState &pipeline_state)
{}
}        // namespace xihe::backend
