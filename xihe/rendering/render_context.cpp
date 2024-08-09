#include "render_context.h"

#include "rendering/render_frame.h"

namespace xihe::rendering
{
RenderContext::RenderContext(backend::Device &device, vk::SurfaceKHR surface, const Window &window, vk::PresentModeKHR present_mode, std::vector<vk::PresentModeKHR> const &present_mode_priority_list, std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list) :
    device_{device}, window_{window}, surface_extent_{window.get_extent().width, window.get_extent().height}
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

	bindless_descriptor_set_ = std::make_unique<backend::BindlessDescriptorSet>(device);

	graphics_queue_ = &device.get_suitable_graphics_queue();
	uint32_t compute_family_index = device.get_queue_family_index(vk::QueueFlagBits::eCompute);

	if (compute_family_index == graphics_queue_->get_family_index())
	{
		LOGI("Device has does not have a dedicated compute queue family.");
		compute_queue_ = graphics_queue_;
	}
	else
	{
		LOGI("Device has async compute queue.");
		compute_queue_ = &device.get_queue(compute_family_index, 0);
	}

	// todo: 这部分可能需要封装
	vk::SemaphoreCreateInfo     semaphore_create_info;
	vk::SemaphoreTypeCreateInfo semaphore_type_create_info;
	semaphore_type_create_info.semaphoreType = vk::SemaphoreType::eTimeline;
	semaphore_create_info.pNext              = &semaphore_type_create_info;

	graphics_semaphore_ = device_.get_handle().createSemaphore(semaphore_create_info);
	compute_semaphore_  = device_.get_handle().createSemaphore(semaphore_create_info);
}

RenderContext::~RenderContext()
{
	device_.get_handle().destroySemaphore(graphics_semaphore_);
	device_.get_handle().destroySemaphore(compute_semaphore_);
}

void RenderContext::prepare(size_t thread_count)
{
	device_.get_handle().waitIdle();

	surface_extent_ = swapchain_->get_extent();

	const vk::Extent3D extent{surface_extent_.width, surface_extent_.height, 1};

	assert(swapchain_);

	for (auto &image_handle : swapchain_->get_images())
	{
		frames_.emplace_back(std::make_unique<rendering::RenderFrame>(device_, thread_count));
	}

	thread_count_ = thread_count;
	prepared_     = true;
}

void RenderContext::recreate()
{
	LOGI("Recreated swapchain");

	vk::Extent2D swapchain_extent = swapchain_->get_extent();
	vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	assert(frames_.size() == swapchain_->get_images().size() && "Frame count does not match swapchain image count");

	for (uint32_t i = 0; i < frames_.size(); ++i)
	{
		for (auto &[rdg_name, create_func] : create_render_target_functions_)
		{
			backend::Image swapchain_image{device_, swapchain_->get_images()[i], extent, swapchain_->get_format(), swapchain_->get_image_usage()};

			auto render_target = create_func(std::move(swapchain_image));
			frames_[i]->update_render_target(rdg_name, std::move(render_target));
		}
	}

	device_.get_resource_cache().clear_framebuffers();
}

void RenderContext::recreate_swapchain()
{
	/*device_.get_handle().waitIdle();
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
	}*/
	throw std::runtime_error("Not implemented");
}

void RenderContext::begin()
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

	/*const auto &queue = device_.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
	return get_active_frame().request_command_buffer(queue, reset_mode, vk::CommandBufferLevel::ePrimary, 0);*/
}

RenderFrame &RenderContext::get_active_frame() const
{
	assert(frame_active_ && "Frame is not active, please call begin_frame");
	return *frames_[active_frame_index_];
}

backend::Device &RenderContext::get_device() const
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
		render_semaphore = submit(*graphics_queue_, command_buffers, acquired_semaphore_, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}
	else
	{
		submit(*graphics_queue_, command_buffers);
	}

	end_frame(render_semaphore);
}

vk::Extent2D const &RenderContext::get_surface_extent() const
{
	return surface_extent_;
}

backend::CommandBuffer & RenderContext::request_graphics_command_buffer(backend::CommandBuffer::ResetMode reset_mode, vk::CommandBufferLevel level, size_t thread_index) const
{
	return get_active_frame().request_command_buffer(*graphics_queue_, reset_mode, level, thread_index);
}

backend::CommandBuffer & RenderContext::request_compute_command_buffer(backend::CommandBuffer::ResetMode reset_mode, vk::CommandBufferLevel level, size_t thread_index) const
{
	return get_active_frame().request_command_buffer(*compute_queue_, reset_mode, level, thread_index);
}

backend::BindlessDescriptorSet *RenderContext::get_bindless_descriptor_set() const
{
	return bindless_descriptor_set_.get();
}

void RenderContext::reset_bindless_index() const
{
	bindless_descriptor_set_->reset_index();
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
		vk::SwapchainKHR   vk_swapchain = swapchain_->get_handle();
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
			result = graphics_queue_->get_handle().presentKHR(present_info);
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
		get_active_frame().release_owned_semaphore(acquired_semaphore_);
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

void RenderContext::update_swapchain(const std::set<vk::ImageUsageFlagBits> &image_usage_flags)
{
	assert(swapchain_);

	device_.get_resource_cache().clear_framebuffers();

	swapchain_ = std::make_unique<backend::Swapchain>(*swapchain_, image_usage_flags);

	recreate();
}

void RenderContext::register_rdg_render_target(const std::string &name, const RdgPass *rdg_pass)
{
	surface_extent_ = swapchain_->get_extent();
	const vk::Extent3D extent{surface_extent_.width, surface_extent_.height, 1};

	assert(frames_.size() == swapchain_->get_images().size() && "Frame count does not match swapchain image count");

	for (size_t i = 0; i < swapchain_->get_images().size(); ++i)
	{
		auto swapchain_image = backend::Image{device_, swapchain_->get_images()[i], extent, swapchain_->get_format(), swapchain_->get_image_usage()};
		auto render_target   = rdg_pass->create_render_target(std::move(swapchain_image));

		auto     image_infos                         = rdg_pass->get_descriptor_image_infos(*render_target);
		uint32_t first_bindless_descriptor_set_index = std::numeric_limits<uint32_t>::max();
		for (auto &image_info : image_infos)
		{
			auto index = get_bindless_descriptor_set()->update(image_info);

			if (index < first_bindless_descriptor_set_index)
			{
				first_bindless_descriptor_set_index = index;
			}
		}
		render_target->set_first_bindless_descriptor_set_index(first_bindless_descriptor_set_index);

		frames_[i]->update_render_target(name, std::move(render_target));
	}
	if (rdg_pass->needs_recreate_rt())
	{
		create_render_target_functions_[name] =
		    [name, rdg_pass](backend::Image &&swapchain_image) {
			    return rdg_pass->create_render_target(std::move(swapchain_image));
		    };
	}
}
}        // namespace xihe::rendering
