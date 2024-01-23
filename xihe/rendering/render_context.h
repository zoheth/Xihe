#pragma once

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

	void prepare(size_t thread_count = 1, RenderTarget::CreateFunc create_render_target_func = RenderTarget::kDefaultCreateFunc);

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