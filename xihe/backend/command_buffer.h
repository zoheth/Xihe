#pragma once

#include "common/helpers.h"
#include "common/vk_common.h"
#include "framebuffer.h"
#include "image.h"
#include "render_pass.h"
#include "rendering/pipeline_state.h"
#include "resources_management/resource_binding_state.h"
#include "vulkan_resource.h"

namespace xihe::backend
{
class CommandPool;
class DescriptorSetLayout;

class CommandBuffer : public VulkanResource<vk::CommandBuffer>
{
  public:
	struct RenderPassBinding
	{
		const backend::RenderPass  *render_pass;
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

	void begin_render_pass(const rendering::RenderTarget                          &render_target,
	                       const std::vector<LoadStoreInfo>               &load_store_infos,
	                       const std::vector<vk::ClearValue>                              &clear_values,
	                       const std::vector<std::unique_ptr<rendering::Subpass>> &subpasses,
	                       vk::SubpassContents                                             contents = vk::SubpassContents::eInline);

	void begin_render_pass(const rendering::RenderTarget &render_target,
	                       const RenderPass        &render_pass,
	                       const Framebuffer       &framebuffer,
	                       const std::vector<vk::ClearValue>     &clear_values,
	                       vk::SubpassContents                    contents = vk::SubpassContents::eInline);

	vk::Result end();

	void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);

	void bind_vertex_buffers(uint32_t                                                          first_binding,
	                         const std::vector<std::reference_wrapper<const backend::Buffer>> &buffers,
	                         const std::vector<vk::DeviceSize>                                &offsets);

	void image_memory_barrier(const backend::ImageView &image_view, const ImageMemoryBarrier &memory_barrier);

	/**
	 * @brief Reset the command buffer to a state where it can be recorded to
	 * @param reset_mode How to reset the buffer, should match the one used by the pool to allocate it
	 */
	vk::Result reset(ResetMode reset_mode);

	void set_viewport_state(const ViewportState &state_info);

	void set_vertex_input_state(const VertexInputState &state_info);

	void set_input_assembly_state(const InputAssemblyState &state_info);

	void set_rasterization_state(const RasterizationState &state_info);

	void set_multisample_state(const MultisampleState &state_info);

	void set_depth_stencil_state(const DepthStencilState &state_info);

	void set_color_blend_state(const ColorBlendState &state_info);

	void set_viewport(uint32_t first_viewport, const std::vector<vk::Viewport> &viewports);

	void set_scissor(uint32_t first_scissor, const std::vector<vk::Rect2D> &scissors);

	void set_line_width(float line_width);

	void set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);

	void set_blend_constants(const std::array<float, 4> &blend_constants);

	void set_depth_bounds(float min_depth_bounds, float max_depth_bounds);

	void bind_pipeline_layout(PipelineLayout &pipeline_layout);

  private:
	void flush(vk::PipelineBindPoint pipeline_bind_point);
	void flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point);
	void flush_pipeline_state(vk::PipelineBindPoint pipeline_bind_point);
	void flush_push_constants();

	const vk::CommandBufferLevel level_{};
	CommandPool                 &command_pool_;

	RenderPassBinding    current_render_pass_{};
	PipelineState        pipeline_state_{};
	ResourceBindingState resource_binding_state_{};

	std::vector<uint8_t> stored_push_constants_{};
	uint32_t             max_push_constants_size_ = {};

	vk::Extent2D last_framebuffer_extent_{};
	vk::Extent2D last_render_area_extent_{};

	bool update_after_bind_ = false;

	std::unordered_map<uint32_t, DescriptorSetLayout const *> descriptor_set_layout_binding_state_;
};
}        // namespace xihe::backend
