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

void RdgPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, backend::CommandBuffer *p_secondary_command_buffer)
{
	if (p_secondary_command_buffer)
	{
		begin_draw(command_buffer, render_target, vk::SubpassContents::eSecondaryCommandBuffers);
		command_buffer.execute_commands({*p_secondary_command_buffer});
	}
	else
	{
		begin_draw(command_buffer, render_target, vk::SubpassContents::eInline);
		draw_subpasses(command_buffer, render_target);
	}

	end_draw(command_buffer, render_target);
}

std::vector<vk::DescriptorImageInfo> RdgPass::get_descriptor_image_infos(RenderTarget &render_target) const
{
	return {};
}

bool RdgPass::use_swapchain_image() const
{
	return use_swapchain_image_;
}

void RdgPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents)
{
	// Pad clear values if they're less than render target attachments
	while (clear_value_.size() < render_target.get_attachments().size())
	{
		clear_value_.push_back(vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f});
	}

	command_buffer.begin_render_pass(render_target, load_store_, clear_value_, subpasses_, contents);
}

void RdgPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	command_buffer.end_render_pass();
}

void RdgPass::add_subpass(std::unique_ptr<Subpass> &&subpass)
{
	subpass->prepare();
	subpasses_.emplace_back(std::move(subpass));
}

void RdgPass::draw_subpasses(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const
{
	assert(!subpasses_.empty() && "Render pipeline should contain at least one sub-pass");

	for (size_t i = 0; i < subpasses_.size(); ++i)
	{

		auto &subpass = subpasses_[i];

		subpass->update_render_target_attachments(render_target);

		if (i != 0)
		{
			command_buffer.next_subpass();
		}

		if (subpass->get_debug_name().empty())
		{
			subpass->set_debug_name(fmt::format("RP subpass #{}", i));
		}
		backend::ScopedDebugLabel subpass_debug_label{command_buffer, subpass->get_debug_name().c_str()};

		subpass->draw(command_buffer);
	}
}
}        // namespace xihe::rendering
