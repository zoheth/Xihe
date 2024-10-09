#pragma once

#include <variant>

#include "rendering/rdg.h"
#include "rendering/render_pipeline.h"
#include "rendering/render_target.h"

// #define RDG_LOG_ENABLED

#ifdef RDG_LOG_ENABLED
#	define RDG_LOG(...) LOGI(__VA_ARGS__)
#else
#	define RDG_LOG(...)
#endif

namespace xihe
{
namespace rendering
{
class RenderPipeline;
class RenderTarget;

using Barrier = std::variant<common::ImageMemoryBarrier, common::BufferMemoryBarrier>;

struct PassInfo
{
	struct Input
	{
		RdgResourceType type;
		std::string     name;
	};
	struct Output
	{
		RdgResourceType                                   type;
		std::string                                       name;
		vk::Format                                        format;
		vk::ImageUsageFlags                               usage{};
		vk::Extent3D                                      override_resolution{};
		std::function<vk::Extent3D(const vk::Extent3D &)> modify_extent{};

		vk::AttachmentLoadOp load_op{vk::AttachmentLoadOp::eClear};
		backend::Sampler    *sampler{nullptr};

		void set_sampler(backend::Sampler *s)
		{
			sampler = s;
			usage |= vk::ImageUsageFlagBits::eSampled;
		}
	};
	std::vector<Input>  inputs;
	std::vector<Output> outputs;
};

class RdgPass
{
  public:
	RdgPass(std::string name, RenderContext &render_context, RdgPassType pass_type, PassInfo &&pass_info);

	RdgPass(RdgPass &&) = default;

	virtual ~RdgPass() = default;

	/// \brief Creates or recreates a render target using a provided swapchain image.
	/// \param swapchain_image The image from the swapchain used to create the render target.
	/// This version of the function is called when the swapchain changes and a new image is available.
	virtual std::unique_ptr<RenderTarget> create_render_target(backend::Image &&swapchain_image) const;

	/// \brief Creates or recreates a render target without a swapchain image.
	/// This version is used when no swapchain image is available or needed.
	virtual RenderTarget *get_render_target() const;

	virtual void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers) = 0;

	virtual std::vector<vk::DescriptorImageInfo> get_descriptor_image_infos(RenderTarget &render_target) const;

	// before pall
	virtual void prepare(backend::CommandBuffer &command_buffer);

	void                                     set_input_image_view(uint32_t index, const backend::ImageView *image_view);
	std::vector<const backend::ImageView *> &get_input_image_views();

	void add_input_memory_barrier(uint32_t index, Barrier &&barrier);
	void add_output_memory_barrier(uint32_t index, Barrier &&barrier);

	// Each RDG pass corresponds to multiple in-flight frames, clean up some states to avoid interference.
	void reset_before_frame();

	void add_release_barrier(const backend::ImageView *image_view, Barrier &&barrier);

	backend::Device &get_device() const;

	const RdgPassType &get_pass_type() const;

	const std::string &get_name() const;

	PassInfo &get_pass_info();

	/// \brief Checks if the render target should be created using a swapchain image.
	/// \return True if a swapchain image should be used, otherwise false.
	bool needs_recreate_rt() const;

	void set_batch_index(int64_t index);
	int64_t get_batch_index() const;

  protected:
	virtual void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents = vk::SubpassContents::eInline);

	virtual void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target);

	std::string name_{};

	RenderContext &render_context_;

	RdgPassType pass_type_{};

	PassInfo pass_info_;

	std::vector<const backend::ImageView *> input_image_views_;

	// std::vector<std::unique_ptr<Subpass>> subpasses_;

	std::vector<common::LoadStoreInfo> load_store_;
	std::vector<vk::ClearValue>        clear_value_;

	RenderTarget::CreateFunc create_render_target_func_{nullptr};

	// Barriers applied before execution to ensure the input resources are in the correct state for reading.
	std::unordered_map<uint32_t, Barrier> input_barriers_;

	// Barriers applied before execution to ensure the output resources are in the correct state for writing.
	std::unordered_map<uint32_t, Barrier> output_barriers_;

	// Barriers applied after execution to release resource ownership for cross-queue synchronization.
	std::unordered_map<const backend::ImageView *, Barrier> release_barriers_;

	std::unique_ptr<RenderTarget> render_target_{nullptr};

	bool needs_recreate_rt_{false};
	bool use_swapchain_image_{false};

	int64_t batch_index_{-1};

};

class RasterRdgPass : public RdgPass
{
  public:
	RasterRdgPass(std::string name, RenderContext &render_context, PassInfo &&pass_info, std::vector<std::unique_ptr<Subpass>> &&subpasses);

	~RasterRdgPass() override = default;

	void prepare(backend::CommandBuffer &command_buffer) override;

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers) override;

	void draw_subpass(backend::CommandBuffer &command_buffer, const RenderTarget &render_target, uint32_t i) const;

	std::vector<std::unique_ptr<Subpass>> &get_subpasses();

	uint32_t get_subpass_count() const;

	const std::vector<common::LoadStoreInfo> &get_load_store() const;

	backend::RenderPass  &get_render_pass() const;
	backend::Framebuffer &get_framebuffer() const;

	/**
	 * @brief Thread index to use for allocating resources
	 */
	void set_thread_index(uint32_t subpass_index, uint32_t thread_index);

  protected:
	void begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents) override;

	void end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target) override;

  private:
	void add_subpass(std::unique_ptr<Subpass> &&subpass);

	std::vector<std::unique_ptr<Subpass>> subpasses_;

	backend::RenderPass  *render_pass_{nullptr};
	backend::Framebuffer *framebuffer_{nullptr};
};

class ComputeRdgPass : public RdgPass
{
  public:
	using ComputeFunction = std::function<void(backend::CommandBuffer &command_buffer, ComputeRdgPass &rdg_pass)>;

	ComputeRdgPass(std::string name, RenderContext &render_context, PassInfo &&pass_info, const std::vector<backend::ShaderSource> &shader_sources);

	~ComputeRdgPass() override = default;

	void set_compute_function(ComputeFunction &&compute_function);

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers) override;

	std::vector<backend::PipelineLayout *> &get_pipeline_layouts();

  private:
	std::vector<backend::PipelineLayout *> pipeline_layouts_;

	ComputeFunction compute_function_{nullptr};
};

}        // namespace rendering
}        // namespace xihe
