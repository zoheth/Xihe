#pragma once
#include <vulkan/vulkan.hpp>

#include "rendering/pipeline_state.h"

namespace xihe::backend
{
class Device;

class Pipeline
{
  public:
	Pipeline(Device &device);

	Pipeline(const Pipeline &) = delete;
	Pipeline(Pipeline &&other) noexcept;

	virtual ~Pipeline();

	Pipeline &operator=(const Pipeline &) = delete;
	Pipeline &operator=(Pipeline &&)      = delete;

	VkPipeline get_handle() const;

	const PipelineState &get_state() const;

  protected:
	Device &device_;

	vk::Pipeline handle_{VK_NULL_HANDLE};

	PipelineState state_;
};

class ComputePipeline : public Pipeline
{
  public:
	ComputePipeline(ComputePipeline &&other) = default;

	~ComputePipeline() override = default;

	ComputePipeline(Device &device, vk::PipelineCache pipeline_cache, PipelineState &pipeline_state);
};

class GraphicsPipeline : public Pipeline
{
  public:
	GraphicsPipeline(GraphicsPipeline &&other) = default;

	~GraphicsPipeline() override = default;

	GraphicsPipeline(Device &device, vk::PipelineCache pipeline_cache, PipelineState &pipeline_state);
};

}        // namespace xihe::backend
