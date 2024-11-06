#pragma once

#include "common/helpers.h"
#include "common/vk_common.h"
#include "framebuffer.h"
#include "image.h"
#include "render_pass.h"
#include "rendering/pipeline_state.h"
#include "resources_management/resource_binding_state.h"
#include "vulkan_resource.h"

namespace xihe
{
struct LightingState;

namespace rendering
{
class Subpass;
}

namespace backend
{
class CommandPool;
class DescriptorSetLayout;
class BindlessDescriptorSet;

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

	vk::Result begin(vk::CommandBufferUsageFlags flags, const backend::RenderPass *render_pass, const backend::Framebuffer *framebuffer, uint32_t subpass_index);

	void init_state(uint32_t subpass_index);

	void begin_render_pass(const rendering::RenderTarget                          &render_target,
	                       const std::vector<common::LoadStoreInfo>               &load_store_infos,
	                       const std::vector<vk::ClearValue>                      &clear_values,
	                       const std::vector<std::unique_ptr<rendering::Subpass>> &subpasses,
	                       vk::SubpassContents                                     contents = vk::SubpassContents::eInline);

	void begin_render_pass(const rendering::RenderTarget     &render_target,
	                       const RenderPass                  &render_pass,
	                       const Framebuffer                 &framebuffer,
	                       const std::vector<vk::ClearValue> &clear_values,
	                       vk::SubpassContents                contents = vk::SubpassContents::eInline);

	void next_subpass();

	void execute_commands(CommandBuffer &secondary_command_buffer);

	void execute_commands(std::vector<CommandBuffer *> &secondary_command_buffers);

	void end_render_pass();

	vk::Result end();

	template <class T>
	void set_specialization_constant(uint32_t constant_id, const T &data)
	{
		set_specialization_constant(constant_id, to_bytes(data));
	}

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void push_constants(const std::vector<uint8_t> &values);

	template <typename T>
	void push_constants(const T &value)
	{
		auto data = to_bytes(value);

		uint32_t size = to_u32(stored_push_constants_.size() + data.size());

		if (size > max_push_constants_size_)
		{
			LOGE("Push constant limit exceeded ({} / {} bytes)", size, max_push_constants_size_);
			throw std::runtime_error("Cannot overflow push constant limit");
		}

		stored_push_constants_.insert(stored_push_constants_.end(), data.begin(), data.end());
	}

	void bind_buffer(const backend::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const backend::ImageView &image_view, const backend::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const backend::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_input(const backend::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_vertex_buffers(uint32_t                                                          first_binding,
	                         const std::vector<std::reference_wrapper<const backend::Buffer>> &buffers,
	                         const std::vector<vk::DeviceSize>                                &offsets);

	void bind_index_buffer(const backend::Buffer &buffer, vk::DeviceSize offset, vk::IndexType index_type);

	void bind_lighting(LightingState &lighting_state, uint32_t set, uint32_t binding);

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

	void set_has_mesh_shader(bool has_mesh_shader);

	void bind_pipeline_layout(PipelineLayout &pipeline_layout);

	RenderPass &get_render_pass(const rendering::RenderTarget &render_target, const std::vector<common::LoadStoreInfo> &load_store_infos, const std::vector<std::unique_ptr<rendering::Subpass>> &subpasses);

	/**
	 * @brief Check that the render area is an optimal size by comparing to the render area granularity
	 */
	bool is_render_size_optimal(const vk::Extent2D &extent, const vk::Rect2D &render_area) const;

	void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);

	void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

	void draw_indexed_indirect(const backend::Buffer &buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride);

	void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

	void dispatch_indirect(const backend::Buffer &buffer, vk::DeviceSize offset);

	void draw_mesh_tasks(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

	void update_buffer(const backend::Buffer &buffer, vk::DeviceSize offset, const std::vector<uint8_t> &data);

	void blit_image(const backend::Image &src_img, const backend::Image &dst_img, const std::vector<vk::ImageBlit> &regions);

	void resolve_image(const backend::Image &src_img, const backend::Image &dst_img, const std::vector<vk::ImageResolve> &regions);

	void copy_buffer(const backend::Buffer &src_buffer, const backend::Buffer &dst_buffer, vk::DeviceSize size);

	void copy_image(const backend::Image &src_img, const backend::Image &dst_img, const std::vector<vk::ImageCopy> &regions);

	void copy_buffer_to_image(const backend::Buffer &buffer, const backend::Image &image, const std::vector<vk::BufferImageCopy> &regions);

	void copy_image_to_buffer(const backend::Image &image, vk::ImageLayout image_layout, const backend::Buffer &buffer, const std::vector<vk::BufferImageCopy> &regions);

	void image_memory_barrier(const backend::ImageView &image_view, const common::ImageMemoryBarrier &memory_barrier) const;

	void buffer_memory_barrier(const backend::Buffer &buffer, vk::DeviceSize offset, vk::DeviceSize size, const common::BufferMemoryBarrier &memory_barrier);

	void set_update_after_bind(bool update_after_bind);

	/*void reset_query_pool(const QueryPool &query_pool, uint32_t first_query, uint32_t query_count);

	void begin_query(const QueryPool &query_pool, uint32_t query, vk::QueryControlFlags flags);

	void end_query(const QueryPool &query_pool, uint32_t query);

	void write_timestamp(vk::PipelineStageFlagBits pipeline_stage, const QueryPool &query_pool, uint32_t query);*/



  private:
	void flush(vk::PipelineBindPoint pipeline_bind_point);
	void flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point);
	void flush_pipeline_state(vk::PipelineBindPoint pipeline_bind_point);
	void flush_push_constants();

	const RenderPassBinding &get_current_render_pass() const;

	uint32_t get_current_subpass_index() const;

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
}        // namespace backend
}        // namespace xihe
