#pragma once

#include "framebuffer.h"
#include "render_pass.h"
#include "vulkan_resource.h"
#include "rendering/pipeline_state.h"
#include "resources_management/resource_binding_state.h"

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

	vk::Result begin(vk::CommandBufferUsageFlags flags, const backend::RenderPass *render_pass, const backend::Framebuffer *framebuffer, uint32_t subpass_info);

	vk::Result end();

	void image_memory_barrier(const ImageView &image_view, const ImageMemoryBarrier &memory_barrier);

	/**
	 * @brief Reset the command buffer to a state where it can be recorded to
	 * @param reset_mode How to reset the buffer, should match the one used by the pool to allocate it
	 */
	vk::Result reset(ResetMode reset_mode);

  private:
	const vk::CommandBufferLevel level_{};
	CommandPool &command_pool_;

	RenderPassBinding current_render_pass_{};
	PipelineState pipeline_state_{};
	ResourceBindingState resource_binding_state_{};

	std::vector<uint8_t> stored_push_constants_{};
	uint32_t             max_push_constants_size_ = {};

	vk::Extent2D last_framebuffer_extent_{};
	vk::Extent2D last_render_area_extent_{};


	bool update_after_bind_ = false;

	std::unordered_map<uint32_t, DescriptorSetLayout const *> descriptor_set_layout_binding_state_;
};
}        // namespace xihe::backend
