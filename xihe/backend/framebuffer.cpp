#include "framebuffer.h"

#include "backend/device.h"
#include "rendering/render_target.h"

namespace xihe::backend
{
Framebuffer::Framebuffer(Device &device, const rendering::RenderTarget &render_target, const RenderPass &render_pass) :
    device_{device},
    extent_{render_target.get_extent()}
{
	std::vector<vk::ImageView> attachments;

	for (auto &view : render_target.get_views())
	{
		attachments.emplace_back(view.get_handle());
	}

	vk::FramebufferCreateInfo framebuffer_info{
	    vk::FramebufferCreateFlags{},
	    render_pass.get_handle(),
	    to_u32(attachments.size()),
	    attachments.data(),
	    extent_.width,
	    extent_.height,
	    1};

	vk::Result result = device.get_handle().createFramebuffer(&framebuffer_info, nullptr, &handle_);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create framebuffer");
	}
}

Framebuffer::Framebuffer(Framebuffer &&other) noexcept :
    device_{other.device_},
    extent_{other.extent_},
    handle_{other.handle_}
{
	other.handle_ = VK_NULL_HANDLE;
}

Framebuffer::~Framebuffer()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		device_.get_handle().destroyFramebuffer(handle_, nullptr);
	}
}

const vk::Extent2D &Framebuffer::get_extent() const
{
	return extent_;
}

vk::Framebuffer Framebuffer::get_handle() const
{
	return handle_;
}
}        // namespace xihe::backend
