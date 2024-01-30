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
	const ShaderModule *shader_module = pipeline_state.
}

GraphicsPipeline::GraphicsPipeline(Device &device, vk::PipelineCache pipeline_cache, PipelineState &pipeline_state)
{}
}        // namespace xihe::backend
