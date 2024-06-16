#include "rdg_pass.h"

#include "rendering/subpass.h"

namespace xihe::rendering
{

std::unique_ptr<RenderTarget> RdgPass::create_render_target(backend::Image &&swapchain_image)
{
	throw std::runtime_error("Not implemented");
}

RenderTarget * RdgPass::get_render_target() const
{
	return nullptr;
}

std::vector<vk::DescriptorImageInfo> RdgPass::get_descriptor_image_infos(RenderTarget &render_target) const
{
	return {};
}

bool                               RdgPass::use_swapchain_image() const
{
	return use_swapchain_image_;
}
}
