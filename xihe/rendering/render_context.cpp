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

void RenderContext::recreate()
{
	LOGI("Recreated swapchain");

	vk::Extent2D swapchain_extent = swapchain_->get_extent();
	vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames_.begin();

	for (auto &image_handle : swapchain_->get_images())
	{
		backend::Image swapchain_image{device_, image_handle, extent, swapchain_->get_format(), swapchain_->get_image_usage()};

		auto render_target = create_render_target_func_(std::move(swapchain_image));

		if (frame_it != frames_.end())
		{
			(*frame_it)->update_render_target(std::move(render_target));
		}
		else
		{
			// Create a new frame if the new swapchain has more images than current frames
			frames_.emplace_back(std::make_unique<RenderFrame>(device_, std::move(render_target), thread_count_));
		}

		++frame_it;
	}

	device_.get_resource_cache().clear_framebuffers();
}

void RenderContext::recreate_swapchain()
{
	device_.get_handle().waitIdle();
	device_.get_resource_cache().clear_framebuffers();

	vk::Extent2D swapchain_extent = swapchain_->get_extent();
	vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames_.begin();

	for (auto &image_handle : swapchain_->get_images())
	{
		backend::Image swapchain_image{device_, image_handle, extent, swapchain_->get_format(), swapchain_->get_image_usage()};
		auto           render_target = create_render_target_func_(std::move(swapchain_image));
		(*frame_it)->update_render_target(std::move(render_target));

		++frame_it;
	}
}

backend::CommandBuffer &RenderContext::begin(backend::CommandBuffer::ResetMode reset_mode)
{
	assert(prepared_ && "RenderContext is not prepared");

	if (!frame_active_)
	{
		begin_frame();
	}

	if (!acquired_semaphore_)
	{
		throw std::runtime_error("Couldn't begin frame");
	}

	const auto &queue = device_.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
	return get_active_frame().request_command_buffer(queue, reset_mode);
}

RenderFrame &RenderContext::get_active_frame() const
{
	assert(frame_active_ && "Frame is not active, please call begin_frame");
	return *frames_[active_frame_index_];
}

backend::Device & RenderContext::get_device() const
{
	return device_;
}

void RenderContext::submit(backend::CommandBuffer &command_buffer)
{
	submit({&command_buffer});
}

void RenderContext::submit(const std::vector<backend::CommandBuffer *> &command_buffers)
{
	assert(frame_active_ && "HPPRenderContext is inactive, cannot submit command buffer. Please call begin()");

	vk::Semaphore render_semaphore;

	if (swapchain_)
	{
		assert(acquired_semaphore_ && "We do not have acquired_semaphore, it was probably consumed?\n");
		render_semaphore = submit(queue_, command_buffers, acquired_semaphore_, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}
	else
	{
		submit(queue_, command_buffers);
	}

	end_frame(render_semaphore);
}

void RenderContext::begin_frame()
{
	if (swapchain_)
	{
		handle_surface_changes();
	}

	assert(!frame_active_ && "Frame is still active, please call end_frame");

	auto &prev_frame = *frames_[active_frame_index_];

	// We will use the acquired semaphore in a different frame context,
	// so we need to hold ownership.
	acquired_semaphore_ = prev_frame.request_semaphore_with_ownership();

	if (swapchain_)
	{
		vk::Result result;
		try
		{
			std::tie(result, active_frame_index_) = swapchain_->acquire_next_image(acquired_semaphore_);
		}
		catch (vk::OutOfDateKHRError & /*err*/)
		{
			result = vk::Result::eErrorOutOfDateKHR;
		}

		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
		{
			bool swapchain_updated = handle_surface_changes(result == vk::Result::eErrorOutOfDateKHR);

			if (swapchain_updated)
			{
				std::tie(result, active_frame_index_) = swapchain_->acquire_next_image(acquired_semaphore_);
			}
		}

		if (result != vk::Result::eSuccess)
		{
			prev_frame.reset();
			return;
		}
	}

	frame_active_ = true;

	get_active_frame().reset();
}

void RenderContext::end_frame(vk::Semaphore semaphore)
{
	assert(frame_active_ && "Frame is not active, please call begin_frame");

	if (swapchain_)
	{
		vk::SwapchainKHR vk_swapchain = swapchain_->get_handle();
		vk::PresentInfoKHR present_info(semaphore, vk_swapchain, active_frame_index_);

		vk::DisplayPresentInfoKHR display_present_info;
		if (device_.is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
		    window_.get_display_present_info(&display_present_info, surface_extent_.width, surface_extent_.height))
		{
			present_info.setPNext(&display_present_info);
		}

		vk::Result result;
		try
		{
			result = queue_.get_handle().presentKHR(present_info);
		}
		catch (vk::OutOfDateKHRError &e)
		{
			result = vk::Result::eErrorOutOfDateKHR;
		}

		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
		{
			handle_surface_changes();
		}

	}

	if (acquired_semaphore_)
	{
		get_active_frame().release_owned_semaphore(semaphore);
		acquired_semaphore_ = nullptr;
	}
	frame_active_ = false;
}

vk::Semaphore RenderContext::submit(const backend::Queue &queue, const std::vector<backend::CommandBuffer *> &command_buffers, vk::Semaphore wait_semaphore, vk::PipelineStageFlags wait_pipeline_stage)
{
	std::vector<vk::CommandBuffer> command_buffer_handles(command_buffers.size(), nullptr);
	std::ranges::transform(command_buffers, command_buffer_handles.begin(),
	                       [](const backend::CommandBuffer *cmd_buf) {
		                       return cmd_buf->get_handle();
	                       });

	RenderFrame &frame = get_active_frame();

	vk::Semaphore signal_semaphore = frame.request_semaphore();

	vk::SubmitInfo submit_info{nullptr, nullptr, command_buffer_handles, signal_semaphore};
	if (wait_semaphore)
	{
		submit_info.setWaitSemaphores(wait_semaphore);
		submit_info.pWaitDstStageMask = &wait_pipeline_stage;
	}

	vk::Fence fence = frame.request_fence();

	queue.get_handle().submit(submit_info, fence);

	return signal_semaphore;
}

void RenderContext::submit(const backend::Queue &queue, const std::vector<backend::CommandBuffer *> &command_buffers)
{
	std::vector<vk::CommandBuffer> command_buffer_handles(command_buffers.size(), nullptr);
	std::ranges::transform(command_buffers, command_buffer_handles.begin(),
	                       [](const backend::CommandBuffer *cmd_buf) {
		                       return cmd_buf->get_handle();
	                       });

	RenderFrame &frame = get_active_frame();

	vk::SubmitInfo submit_info{nullptr, nullptr, command_buffer_handles};

	vk::Fence fence = frame.request_fence();

	queue.get_handle().submit(submit_info, fence);
}

bool RenderContext::handle_surface_changes(bool force_update)
{
	if (!swapchain_)
	{
		LOGW("Can't handle surface changes in headless mode, skipping.");
		return false;
	}

	const vk::SurfaceCapabilitiesKHR surface_capabilities = device_.get_gpu().get_handle().getSurfaceCapabilitiesKHR(swapchain_->get_surface());

	if (surface_capabilities.currentExtent.width == 0xFFFFFFFF)
	{
		return false;
	}

	if (surface_capabilities.currentExtent.width != surface_extent_.width || surface_capabilities.currentExtent.height != surface_extent_.height)
	{
		// Recreate swapchain
		device_.get_handle().waitIdle();

		update_swapchain(surface_capabilities.currentExtent);

		surface_extent_ = surface_capabilities.currentExtent;

		return true;
	}

	return false;
}

void RenderContext::update_swapchain(const vk::Extent2D &extent)
{
	device_.get_resource_cache().clear_framebuffers();

	swapchain_ = std::make_unique<backend::Swapchain>(*swapchain_, extent);

	recreate();
}
}        // namespace xihe::rendering
