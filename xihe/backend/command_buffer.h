#pragma once

#include "framebuffer.h"
#include "render_pass.h"
#include "vulkan_resource.h"
#include "rendering/pipeline_state.h"

namespace xihe::backend
{
class CommandPool;
class DescriptorSetLayout;

class CommandBuffer : public VulkanResource<vk::CommandBuffer>
{
  public:
	struct RenderPassBinding
	{
		const backend::RenderPass *render_pass;
		const backend::Framebuffer *framebuffer;
	};

	enum class ResetMode
	{
		kResetPool,
		kResetIndividually,
		kAlwaysAllocate,
	};

  public:
	CommandBuffer(CommandPool &command_pool, vk::CommandBufferLevel level);
	CommandBuffer(const CommandBuffer &) = delete;
	CommandBuffer(CommandBuffer &&other) noexcept;

	~CommandBuffer() override;

	CommandBuffer &operator=(const CommandBuffer &) = delete;
	CommandBuffer &operator=(CommandBuffer &&)      = delete;

	vk::Result begin(vk::CommandBufferUsageFlags flags, CommandBuffer *primary_cmd_buf = nullptr);

  private:
	const vk::CommandBufferLevel level_{};
	CommandPool &command_pool_;

	RenderPassBinding current_render_pass_{};
	PipelineState pipeline_state_{};
	ResourceBindingState

	uint32_t max_push_constant_size_{0};
};
}        // namespace xihe::backend
