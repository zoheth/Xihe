#pragma once
#include "vulkan_resource.h"

namespace xihe::backend
{
class CommandPool;
class DescriptorSetLayout;

class CommandBuffer : public VulkanResource<vk::CommandBuffer>
{
  public:
	enum class ResetMode
	{
		kResetPool,
		kResetIndividually,
		kAlwaysAllocate,
	};

  public:
	CommandBuffer(CommandPool &command_pool, vk::CommandBufferLevel level);
	CommandBuffer(const CommandBuffer &) = delete;
	// CommandBuffer(CommandBuffer &&other) noexcept;

	~CommandBuffer() override;

	CommandBuffer &operator=(const CommandBuffer &) = delete;
	CommandBuffer &operator=(CommandBuffer &&)      = delete;

  private:
	const vk::CommandBufferLevel level_;

	CommandPool &command_pool_;

	uint32_t max_push_constant_size_{0};
};
}        // namespace xihe::backend
