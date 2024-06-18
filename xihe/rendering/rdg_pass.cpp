#include "rdg_pass.h"

#include "render_context.h"
#include "rendering/subpass.h"

namespace xihe::rendering
{
RdgPass::RdgPass(const std::string &name, RenderContext &render_context) :
    name_{name}, render_context_{render_context}
{}

std::unique_ptr<RenderTarget> RdgPass::create_render_target(backend::Image &&swapchain_image)
{
	throw std::runtime_error("Not implemented");
}

RenderTarget *RdgPass::get_render_target() const
{
	if (use_swapchain_image())
	{
		return &render_context_.get_active_frame().get_render_target(name_);
	}
	assert(render_target_ && "If use_swapchain_image returns false, the render_target_ must be created during initialization.");
	return render_target_.get();
}

std::vector<vk::DescriptorImageInfo> RdgPass::get_descriptor_image_infos(RenderTarget &render_target) const
{
	return {};
}

bool RdgPass::use_swapchain_image() const
{
	return use_swapchain_image_;
}
}        // namespace xihe::rendering
