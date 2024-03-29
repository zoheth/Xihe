#include "render_context.h"

#include "rendering/render_frame.h"

namespace xihe::rendering
{
RenderContext::RenderContext(backend::Device &device, vk::SurfaceKHR surface, const Window &window, vk::PresentModeKHR present_mode, std::vector<vk::PresentModeKHR> const &present_mode_priority_list, std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list) :
    device_{device}, window_{window}, surface_extent_{window.get_extent().width, window.get_extent().height}, queue_{device.get_suitable_graphics_queue()}
{
	vk::SurfaceCapabilitiesKHR surface_capabilities = device_.get_gpu().get_handle().getSurfaceCapabilitiesKHR(surface);

	assert(surface);

	if (surface_capabilities.currentExtent.width == 0xFFFFFFFF)
	{
		swapchain_ =
		    std::make_unique<backend::Swapchain>(device, surface, present_mode, present_mode_priority_list, surface_format_priority_list, surface_extent_);
	}
	else
	{
		swapchain_ = std::make_unique<backend::Swapchain>(device, surface, present_mode, present_mode_priority_list, surface_format_priority_list);
	}
}

void RenderContext::prepare(size_t thread_count, RenderTarget::CreateFunc create_render_target_func)
{
	device_.get_handle().waitIdle();

	surface_extent_ = swapchain_->get_extent();

	const vk::Extent3D extent{surface_extent_.width, surface_extent_.height, 1};

	assert(swapchain_);

	for (auto &image_handle : swapchain_->get_images())
	{
		auto swapchain_image = backend::Image{device_, image_handle, extent, swapchain_->get_format(), swapchain_->get_image_usage()};
		auto render_target   = create_render_target_func(std::move(swapchain_image));
		frames_.emplace_back(std::make_unique<rendering::RenderFrame>(device_, std::move(render_target), thread_count));
	}

	create_render_target_func_ = create_render_target_func;
	thread_count_              = thread_count;
	prepared_                  = true;
}

backend::CommandBuffer &RenderContext::begin(backend::CommandBuffer::ResetMode reset_mode)
{
	assert(prepared_ && "RenderContext is not prepared");

	if (!frame_active_)
	{
		begin_frame();
	}
}

void RenderContext::begin_frame()
{
	handle_surface_changes();
}

void RenderContext::handle_surface_changes(bool force_update)
{
	const vk::SurfaceCapabilitiesKHR surface_capabilities = device_.get_gpu().get_handle().getSurfaceCapabilitiesKHR(swapchain_->get_surface());

	if (surface_capabilities.currentExtent.width == 0xFFFFFFFF)
	{
		return;
	}

	if (surface_capabilities.currentExtent.width != surface_extent_.width || surface_capabilities.currentExtent.height != surface_extent_.height)
	{
		// Recreate swapchain
		device_.get_handle().waitIdle();

		

		surface_extent_ = surface_capabilities.currentExtent;
	}
}

void RenderContext::update_swapchain(const vk::Extent2D &extent, const vk::SurfaceTransformFlagBitsKHR transform)
{
	device_.get_resource_cache()
}
}        // namespace xihe::rendering
