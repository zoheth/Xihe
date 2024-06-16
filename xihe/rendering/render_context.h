#pragma once

#include "backend/command_buffer.h"
#include "backend/device.h"
#include "backend/swapchain.h"
#include "platform/window.h"
#include "rendering/rdg_pass.h"
#include "rendering/rdg_passes/main_pass.h"
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
	 */
	void prepare(size_t thread_count = 1);

	void recreate();

	void recreate_swapchain();

	backend::CommandBuffer &begin(backend::CommandBuffer::ResetMode reset_mode = backend::CommandBuffer::ResetMode::kResetPool);

	RenderFrame &get_active_frame() const;

	backend::Device &get_device() const;

	void submit(backend::CommandBuffer &command_buffer);

	void submit(const std::vector<backend::CommandBuffer *> &command_buffers);

	vk::Extent2D const &get_surface_extent() const;

	backend::BindlessDescriptorSet *get_bindless_descriptor_set() const;

	RenderTarget &get_render_target(std::string name) const;

	void update_rdg_bindless_descriptor_set();

	void reset_bindless_index() const;

	void begin_frame();

	void end_frame(vk::Semaphore semaphore);

	vk::Semaphore submit(const backend::Queue                        &queue,
	                     const std::vector<backend::CommandBuffer *> &command_buffers,
	                     vk::Semaphore                                wait_semaphore,
	                     vk::PipelineStageFlags                       wait_pipeline_stage);

	void submit(const backend::Queue &queue, const std::vector<backend::CommandBuffer *> &command_buffers);

	bool handle_surface_changes(bool force_update = false);

	void update_swapchain(const vk::Extent2D &extent);

	void update_swapchain(const std::set<vk::ImageUsageFlagBits> &image_usage_flags);

	template <typename T, typename... Args>
	void add_pass(std::string name, Args &&...args);

	void render(backend::CommandBuffer &command_buffer);

	static void set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent);

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

	vk::SurfaceTransformFlagBitsKHR pre_transform_{vk::SurfaceTransformFlagBitsKHR::eIdentity};

	size_t thread_count_{1};

	std::unique_ptr<backend::BindlessDescriptorSet> bindless_descriptor_set_;

	std::unordered_map<std::string, std::unique_ptr<RdgPass>> rdg_passes_{};
};

/**
 * \brief 
 * \tparam T Subclass of RdgPass.
 * \tparam Args 
 * \param name 
 * \param args Construction parameters for T, excluding render context.
 */
template <typename T, typename... Args>
void RenderContext::add_pass(std::string name, Args &&...args)
{
	static_assert(std::is_base_of_v<rendering::RdgPass, T>, "T must be a derivative of RenderPass");

	if (rdg_passes_.contains(name))
	{
		throw std::runtime_error{"Pass with name " + name + " already exists"};
	}


	rdg_passes_.emplace(name, std::make_unique<T>(*this, std::forward<Args>(args)...));

	surface_extent_ = swapchain_->get_extent();
	const vk::Extent3D extent{surface_extent_.width, surface_extent_.height, 1};

	assert(frames_.size() == swapchain_->get_images().size() && "Frame count does not match swapchain image count");

	for (size_t i = 0 ; i<swapchain_->get_images().size(); ++i)
	{
		if (rdg_passes_[name]->use_swapchain_image())
		{
			auto swapchain_image = backend::Image{device_, swapchain_->get_images()[i], extent, swapchain_->get_format(), swapchain_->get_image_usage()};
			auto render_target   = rdg_passes_[name]->create_render_target(std::move(swapchain_image));
			frames_[i]->update_render_target(name, std::move(render_target));
		}
		/*else
		{
			auto render_target = rdg_passes_[name]->create_render_target();
			frames_[i]->update_render_target(name, std::move(render_target));
		}*/
	}
}
}        // namespace xihe::rendering