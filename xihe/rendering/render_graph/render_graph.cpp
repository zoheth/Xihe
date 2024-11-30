#include "render_graph.h"

#include "rendering/render_frame.h"

#include <ranges>

namespace xihe::rendering
{
void set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D({}, extent));
}

PassNode::PassNode(std::string name, PassType type, PassInfo &&pass_info, std::unique_ptr<RenderPass> &&render_pass) :
    name_{std::move(name)}, type_{type}, pass_info_{std::move(pass_info)}, render_pass_{std::move(render_pass)}
{
}

void PassNode::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, RenderFrame &render_frame)
{
	backend::ScopedDebugLabel subpass_debug_label{command_buffer, name_.c_str()};

	std::vector<ShaderBindable> shader_bindable(inputs_.size());

	// todo input barriers
	for (const auto &[index, input_info] : inputs_)
	{
		shader_bindable[index] = input_info.resource;

		if (std::holds_alternative<backend::Buffer *>(input_info.resource))
		{
			// command_buffer.buffer_memory_barrier(std::get<backend::Buffer *>(input_info.resource), input_info.barrier);
		}
		else
		{
			command_buffer.image_memory_barrier(*std::get<backend::ImageView *>(input_info.resource), std::get<common::ImageMemoryBarrier> (input_info.barrier));
		}
	}

	auto &output_views = render_target.get_views();
	for (const auto &[index, barrier] : output_barriers_)
	{
		if (std::holds_alternative<common::ImageMemoryBarrier>(barrier))
		{
			command_buffer.image_memory_barrier(output_views[index], std::get<common::ImageMemoryBarrier>(barrier));
		}
		else
		{
		}
	}

	command_buffer.begin_rendering(render_target);

	render_pass_->execute(command_buffer, render_frame, shader_bindable);

	command_buffer.end_rendering();

	if (!render_target_)
	{
		auto                      &views = render_target.get_views();
		common::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		memory_barrier.new_layout      = vk::ImageLayout::ePresentSrcKHR;
		memory_barrier.src_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
		memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eBottomOfPipe;

		command_buffer.image_memory_barrier(views[0], memory_barrier);
	}

	for (auto &[view, barrier] : release_barriers_)
	{
		if (std::holds_alternative<common::ImageMemoryBarrier>(barrier))
		{
			command_buffer.image_memory_barrier(*view, std::get<common::ImageMemoryBarrier>(barrier));
		}
		else
		{
		}
	}
}

void PassNode::set_render_target(std::unique_ptr<RenderTarget> &&render_target)
{
	render_target_ = std::move(render_target);
}

RenderTarget * PassNode::get_render_target()
{
	return render_target_.get();
}

void PassNode::add_input_info(uint32_t index, Barrier &&barrier, std::variant<backend::Buffer*, backend::ImageView*> &&resource)
{
	assert(index < pass_info_.inputs.size());
	if (std::holds_alternative<backend::Buffer *>(resource))
	{
		assert(std::holds_alternative<common::BufferMemoryBarrier>(barrier));
	}
	else
	{
		assert(std::holds_alternative<common::ImageMemoryBarrier>(barrier));
	}
	inputs_[index] = {barrier, resource};
}

void PassNode::add_output_memory_barrier(uint32_t index, Barrier &&barrier)
{
	output_barriers_[index] = barrier;
}

void PassNode::add_release_barrier(const backend::ImageView *image_view, Barrier &&barrier)
{
	release_barriers_[image_view] = barrier;
}

RenderGraph::RenderGraph(RenderContext &render_context) : render_context_{render_context}
{}

void RenderGraph::execute()
{
	render_context_.begin_frame();

	size_t batch_count = pass_batches_.size();
	for (size_t i = 0; i < batch_count; ++i)
	{
		bool is_first = (i == 0);
		bool is_last  = (i == batch_count - 1);
		if (pass_batches_[i].type == PassType::kRaster)
		{
			execute_raster_batch(pass_batches_[i], is_first, is_last);
		}
		else if (pass_batches_[i].type == PassType::kCompute)
		{
			execute_compute_batch(pass_batches_[i], is_first, is_last);
		}
	}
}

void RenderGraph::add_pass_node(PassNode &&pass_node)
{
	pass_nodes_.push_back(std::move(pass_node));
}

void RenderGraph::execute_raster_batch(PassBatch &pass_batch, bool is_first, bool is_last)
{
	auto &command_buffer = render_context_.request_graphics_command_buffer(
	    backend::CommandBuffer::ResetMode::kResetPool,
	    vk::CommandBufferLevel::ePrimary, 0);

	command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	for (const auto pass_node : pass_batch.pass_nodes)
	{
		RenderTarget *render_target = pass_node->get_render_target();

		if (!render_target)
		{
			render_target = &render_context_.get_active_frame().get_render_target();
		}

		set_viewport_and_scissor(command_buffer, render_target->get_extent());

		pass_node->execute(command_buffer, *render_target, render_context_.get_active_frame());
	}

	command_buffer.end();

	const auto     last_wait_batch      = pass_batch.wait_batch_index;
	const uint64_t wait_semaphore_value = last_wait_batch >= 0 ? pass_batches_[last_wait_batch].signal_semaphore_value : 0;

	render_context_.graphics_submit(
	    {&command_buffer},        // list of command buffers
	    pass_batch.signal_semaphore_value,
	    wait_semaphore_value,
	    is_first,
	    is_last);
}

void RenderGraph::execute_compute_batch(PassBatch &pass_batch, bool is_first, bool is_last)
{
	
}
}
