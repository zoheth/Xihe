#pragma once

#include <vulkan/vulkan.hpp>

namespace xihe
{
namespace rendering
{
class RenderTarget;
}

namespace backend
{
class Device;
class RenderPass;

class Framebuffer
{
  public:
	Framebuffer(Device &device, const rendering::RenderTarget &render_target, const RenderPass &render_pass);

	Framebuffer(const Framebuffer &) = delete;

	Framebuffer(Framebuffer &&other) noexcept;

	~Framebuffer();

	Framebuffer &operator=(const Framebuffer &) = delete;
	Framebuffer &operator=(Framebuffer &&)      = delete;

	const vk::Extent2D &get_extent() const;

	vk::Framebuffer     get_handle() const;

private:
	Device &device_;

	vk::Framebuffer handle_{VK_NULL_HANDLE};

	vk::Extent2D extent_;
};
}        // namespace backend
}        // namespace xihe
