#include "rdg_pass.h"

#include "render_context.h"
#include "rendering/subpass.h"

namespace xihe::rendering
{
RdgPass::RdgPass(std::string name, const RdgPassType pass_type, RenderContext &render_context) :
    name_{std::move(name)}, pass_type_(pass_type), render_context_{render_context}
{}

std::unique_ptr<RenderTarget> RdgPass::create_render_target(backend::Image &&swapchain_image) const
{
	throw std::runtime_error("Not implemented");
}

RenderTarget *RdgPass::get_render_target() const
{
	return &render_context_.get_active_frame().get_render_target(name_);
	/*if (use_swapchain_image())
	{
		return &render_context_.get_active_frame().get_render_target(name_);
	}
	assert(render_target_ && "If use_swapchain_image returns false, the render_target_ must be created during initialization.");
	return render_target_.get();*/
}

void RdgPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers)
{
	if (pass_type_ == RdgPassType::kRaster)
	{
		if (!secondary_command_buffers.empty())
		{
			begin_draw(command_buffer, render_target, vk::SubpassContents::eSecondaryCommandBuffers);
			for (size_t i = 0; i < subpasses_.size(); ++i)
			{
				subpasses_[i]->update_render_target_attachments(render_target);
				if (i != 0)
				{
					command_buffer.get_handle().nextSubpass(vk::SubpassContents::eSecondaryCommandBuffers);
				}

				command_buffer.execute_commands({*secondary_command_buffers[i]});
			}
		}
		else
		{
			begin_draw(command_buffer, render_target, vk::SubpassContents::eInline);
			for (size_t i = 0; i < subpasses_.size(); ++i)
			{
				subpasses_[i]->update_render_target_attachments(render_target);
				if (i != 0)
				{
					command_buffer.next_subpass();
				}

				draw_subpass(command_buffer, render_target, i);
			}
		}

		end_draw(command_buffer, render_target);	
	}
}

std::vector<vk::DescriptorImageInfo> RdgPass::get_descriptor_image_infos(RenderTarget &render_target) const
{
	return {};
}

void RdgPass::prepare(backend::CommandBuffer &command_buffer)
{
	if (pass_type_ == RdgPassType::kRaster)
	{
		render_pass_ = &command_buffer.get_render_pass(*get_render_target(), load_store_, subpasses_);
		framebuffer_ = &get_device().get_resource_cache().request_framebuffer(*get_render_target(), *render_pass_);
	}
}

backend::Device &RdgPass::get_device() const
{
	return render_context_.get_device();
}

std::vector<std::unique_ptr<Subpass>> &RdgPass::get_subpasses()
{
	return subpasses_;
}

uint32_t RdgPass::get_subpass_count() const
{
	return subpasses_.size();
}

const std::vector<common::LoadStoreInfo> &RdgPass::get_load_store() const
{
	return load_store_;
}

backend::RenderPass & RdgPass::get_render_pass() const
{
	assert(render_pass_ && "");
	return *render_pass_;
}

backend::Framebuffer & RdgPass::get_framebuffer() const
{
	assert(framebuffer_ && "");
	return *framebuffer_;
}

void RdgPass::set_thread_index(uint32_t subpass_index, uint32_t thread_index)
{
	assert(subpass_index < subpasses_.size());
	subpasses_[subpass_index]->set_thread_index(thread_index);
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

	if (render_pass_ && framebuffer_)
	{
		command_buffer.begin_render_pass(render_target, *render_pass_, *framebuffer_, clear_value_, contents);
	}
	else
	{
		command_buffer.begin_render_pass(render_target, load_store_, clear_value_, subpasses_, contents);
	}
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

void RdgPass::draw_subpass(backend::CommandBuffer &command_buffer, const RenderTarget &render_target, uint32_t i) const
{
	auto &subpass = subpasses_[i];

	subpass->set_render_target(&render_target);

	if (subpass->get_debug_name().empty())
	{
		subpass->set_debug_name(fmt::format("RP subpass #{}", i));
	}
	backend::ScopedDebugLabel subpass_debug_label{command_buffer, subpass->get_debug_name().c_str()};

	subpass->draw(command_buffer);
}
}        // namespace xihe::rendering
