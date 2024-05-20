#pragma once

#include "backend/command_buffer.h"
#include "backend/device.h"
#include "backend/swapchain.h"
#include "platform/window.h"
#include "rendering/render_target.h"

namespace xihe::rendering
{
class RenderContext
{
  public:
	RenderContext(backend::Device                         &device,
	              vk::SurfaceKHR                           surface,
	              const Window                            &window,
	              vk::PresentModeKHR                       present_mode                 = vk::PresentModeKHR::eFifo,
	              std::vector<vk::PresentModeKHR> const   &present_mode_priority_list   = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
	              std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list = {
	                  {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}, {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}});

	RenderContext(const RenderContext &) = delete;
	RenderContext(RenderContext &&)      = delete;

	virtual ~RenderContext() = default;

	RenderContext &operator=(const RenderContext &) = delete;
	RenderContext &operator=(RenderContext &&)      = delete;

	/**
	 * \brief Prepares the RenderFrames for rendering
	 * \param thread_count The number of threads in the application, necessary to allocate this many resource pools for each RenderFrame
	 * \param create_render_target_func A function delegate, used to create a RenderTarget
	 */
	void prepare(size_t thread_count = 1, RenderTarget::CreateFunc create_render_target_func = RenderTarget::kDefaultCreateFunc);

	void recreate();

	void recreate_swapchain();

	backend::CommandBuffer &begin(backend::CommandBuffer::ResetMode reset_mode = backend::CommandBuffer::ResetMode::kResetPool);

	RenderFrame &get_active_frame() const;

	backend::Device &get_device() const;

	void submit(backend::CommandBuffer &command_buffer);

	void submit(const std::vector<backend::CommandBuffer *> &command_buffers);

  private:
	void begin_frame();

	void end_frame(vk::Semaphore semaphore);

	vk::Semaphore submit(const backend::Queue                       & queue,
	                     const std::vector<backend::CommandBuffer *> &command_buffers,
	                     vk::Semaphore                                wait_semaphore,
	                     vk::PipelineStageFlags                       wait_pipeline_stage);

	void submit(const backend::Queue &queue, const std::vector<backend::CommandBuffer *> &command_buffers);

	bool handle_surface_changes(bool force_update = false);

	void update_swapchain(const vk::Extent2D &extent);

  private:
	backend::Device &device_;

	const Window &window_;
	vk::Extent2D  surface_extent_;

	const backend::Queue &queue_;

	std::unique_ptr<backend::Swapchain> swapchain_;

	std::vector<std::unique_ptr<RenderFrame>> frames_;

	vk::Semaphore acquired_semaphore_;

	bool     prepared_{false};
	bool     frame_active_{false};
	uint32_t active_frame_index_{0};

	RenderTarget::CreateFunc create_render_target_func_ = RenderTarget::kDefaultCreateFunc;

	vk::SurfaceTransformFlagBitsKHR pre_transform_{vk::SurfaceTransformFlagBitsKHR::eIdentity};

	size_t thread_count_{1};
};
}        // namespace xihe::rendering