#include "rdg_pass.h"

#include "rendering/subpass.h"

namespace xihe::rendering
{

std::unique_ptr<RenderTarget> RdgPass::create_render_target(backend::Image &&swapchain_image)
{
	throw std::runtime_error("Not implemented");
}

std::unique_ptr<RenderTarget> RdgPass::create_render_target()
{
	throw std::runtime_error("Not implemented");
}

bool RdgPass::use_swapchain_image() const
{
	return use_swapchain_image_;
}
}
