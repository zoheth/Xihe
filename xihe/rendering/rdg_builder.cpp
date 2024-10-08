#include "rdg_builder.h"

namespace xihe::rendering
{

void set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D({}, extent));
}

void SecondaryDrawTask::init(backend::CommandBuffer *command_buffer, RasterRdgPass const *pass, uint32_t subpass_index)
{
	this->command_buffer = command_buffer;
	this->pass           = pass;
	this->subpass_index  = subpass_index;
}

void SecondaryDrawTask::ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
{
	const RenderTarget *render_target = pass->get_render_target();

	command_buffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &pass->get_render_pass(), &pass->get_framebuffer(), subpass_index);
	command_buffer->init_state(subpass_index);
	set_viewport_and_scissor(*command_buffer, render_target->get_extent());
	pass->draw_subpass(*command_buffer, *render_target, subpass_index);
	command_buffer->end();
}

RdgBuilder::RdgBuilder(RenderContext &render_context) :
    render_context_{render_context}
{
}

void RdgBuilder::add_raster_pass(const std::string &name, PassInfo &&pass_info, std::vector<std::unique_ptr<Subpass>> &&subpasses)
{
	/*if (rdg_passes_.contains(name))
	{
	    throw std::runtime_error{"Pass with name " + name + " already exists"};
	}*/

	rdg_passes_.push_back(std::make_unique<RasterRdgPass>(name, render_context_, std::move(pass_info), std::move(subpasses)));

	render_context_.register_rdg_render_target(name, rdg_passes_.back().get());
}

void RdgBuilder::add_compute_pass(const std::string &name, PassInfo &&pass_info, const std::vector<backend::ShaderSource> &shader_sources, ComputeRdgPass::ComputeFunction &&compute_function)
{
	rdg_passes_.push_back(std::make_unique<ComputeRdgPass>(name, render_context_, std::move(pass_info), shader_sources));
	dynamic_cast<ComputeRdgPass *>(rdg_passes_.back().get())->set_compute_function(std::move(compute_function));

	render_context_.register_rdg_render_target(name, rdg_passes_.back().get());
}

void RdgBuilder::compile()
{
	build_pass_batches();
	
}

void RdgBuilder::execute()
{
	render_context_.begin_frame();
	compile();
#if 1
	size_t batch_count = pass_batches_.size();
	for (size_t i = 0; i < batch_count; ++i)
	{
		auto &batch = pass_batches_[i];

		backend::CommandBuffer *command_buffer;

		// Request a command buffer based on the pass batch type
		if (batch.type == RdgPassType::kRaster)
		{
			command_buffer = &render_context_.request_graphics_command_buffer(
			    backend::CommandBuffer::ResetMode::kResetPool,
			    vk::CommandBufferLevel::ePrimary, 0);
		}
		else if (batch.type == RdgPassType::kCompute)
		{
			command_buffer = &render_context_.request_compute_command_buffer(
			    backend::CommandBuffer::ResetMode::kResetPool,
			    vk::CommandBufferLevel::ePrimary, 0);
		}
		else
		{
			throw std::runtime_error("Unsupported pass type in pass batch.");
		}

		// Begin the command buffer
		command_buffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Execute each pass in the batch
		for (RdgPass *rdg_pass : batch.passes)
		{
			RDG_LOG("Executing pass: {}", rdg_pass->get_name().c_str());
			RenderTarget *render_target = rdg_pass->get_render_target();

			if (batch.type == RdgPassType::kRaster)
			{
				set_viewport_and_scissor(*command_buffer, render_target->get_extent());
			}

			// Execute the pass
			rdg_pass->execute(*command_buffer, *render_target, {});
		}

		// End the command buffer
		command_buffer->end();

		// Determine if this is the first or last submission
		bool is_first_submission = (i == 0);
		bool is_last_submission  = (i == batch_count - 1 && batch.type == RdgPassType::kRaster);

		const auto     last_wait_batch      = batch.wait_batch_index;
		const uint64_t wait_semaphore_value = last_wait_batch>=0? pass_batches_[last_wait_batch].signal_semaphore_value : 0;

		// Submit the command buffer with appropriate semaphores
		if (batch.type == RdgPassType::kRaster)
		{
			render_context_.graphics_submit(
			    {command_buffer},        // list of command buffers
			    batch.signal_semaphore_value,
			    wait_semaphore_value,
			    is_first_submission,
			    is_last_submission);
		}
		else if (batch.type == RdgPassType::kCompute)
		{
			render_context_.compute_submit(
			    {command_buffer},        // list of command buffers
			    batch.signal_semaphore_value,
			    wait_semaphore_value);
		}
	}
#else

	enki::TaskScheduler scheduler;
	scheduler.Initialize();

	auto        reset_mode = backend::CommandBuffer::ResetMode::kResetPool;
	const auto &queue      = render_context_.get_device().get_suitable_graphics_queue();

	std::unordered_map<int, std::vector<SecondaryDrawTask>> rdg_pass_tasks;

	uint32_t thread_index = 1;

	for (auto &index : pass_order_)
	{
		auto &rdg_pass = rdg_passes_.at(index);

		if (rdg_pass->get_pass_type() == kRaster)
		{
			auto *raster_pass     = dynamic_cast<RasterRdgPass *>(rdg_pass.get());
			rdg_pass_tasks[index] = std::vector<SecondaryDrawTask>(raster_pass->get_subpass_count());

			rdg_pass->prepare(command_buffer);

			for (uint32_t i = 0; i < raster_pass->get_subpass_count(); ++i)
			{
				auto &secondary_command_buffer = render_context_.get_active_frame().request_command_buffer(queue,
				                                                                                           reset_mode,
				                                                                                           vk::CommandBufferLevel::eSecondary,
				                                                                                           thread_index);

				raster_pass->set_thread_index(i, thread_index);

				rdg_pass_tasks[index][i].init(&secondary_command_buffer, raster_pass, i);

				scheduler.AddTaskSetToPipe(&rdg_pass_tasks[index][i]);
				thread_index++;
			}
		}
	}

	uint32_t pass_index = 0;

	for (auto &index : pass_order_)
	{
		auto &rdg_pass = rdg_passes_.at(index);

		std::vector<backend::CommandBuffer *> secondary_command_buffers;

		if (rdg_pass->get_pass_type() == kRaster)
		{
			auto *raster_pass = dynamic_cast<RasterRdgPass *>(rdg_pass.get());
			for (uint32_t i = 0; i < raster_pass->get_subpass_count(); ++i)
			{
				scheduler.WaitforTask(&rdg_pass_tasks[index][i]);
				secondary_command_buffers.push_back(rdg_pass_tasks[index][i].command_buffer);
			}
		}

		rdg_pass->execute(command_buffer, *rdg_pass->get_render_target(), secondary_command_buffers);

		pass_index++;
	}

	command_buffer.end();

	render_context_.submit(command_buffer);

#endif
}

bool RdgBuilder::setup_memory_barrier(const ResourceState &state, const RdgPass *rdg_pass, common::ImageMemoryBarrier &barrier) const
{
	RDG_LOG("    Setting up memory barrier for pass: {}", rdg_pass->get_name().c_str());

	RdgPass *prev_pass = rdg_passes_[state.prev_pass].get();

	if (state.prev_pass != state.producer_pass && prev_pass->get_pass_type()==rdg_pass->get_pass_type())
	{
		return false;
	}

	barrier.new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.dst_access_mask = vk::AccessFlagBits2::eShaderRead;
	if (rdg_pass->get_pass_type() == RdgPassType::kCompute)
	{
		barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
	}
	else if (rdg_pass->get_pass_type() == RdgPassType::kRaster)
	{
		barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eFragmentShader;
	}

	if (rdg_pass->get_pass_type() != prev_pass->get_pass_type())
	{
		RDG_LOG("      Changing queue family, prev pass: {}", prev_pass->get_name().c_str());
		common::ImageMemoryBarrier release_barrier{};
		if (rdg_pass->get_pass_type() == RdgPassType::kCompute)
		{
			release_barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);
			release_barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);
			barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);
			barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);
		}
		else if (rdg_pass->get_pass_type() == RdgPassType::kRaster)
		{
			release_barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);
			release_barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);
			barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);
			barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);
		}
		release_barrier.old_layout = state.prev_layout;
		release_barrier.new_layout      = state.prev_layout;
		release_barrier.src_stage_mask = state.prev_stage_mask;
		release_barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eBottomOfPipe;
		release_barrier.src_access_mask = state.prev_access_mask;
		release_barrier.dst_access_mask = vk::AccessFlagBits2::eNone;
		prev_pass->add_release_barrier(state.image_view, release_barrier);

		barrier.old_layout      = state.prev_layout;
		barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eBottomOfPipe;
		barrier.src_access_mask = vk::AccessFlagBits2::eNone;

		return true;
	}

	barrier.old_layout      = state.prev_layout;
	barrier.src_stage_mask  = state.prev_stage_mask;
	barrier.src_access_mask = state.prev_access_mask;

	return true;
}

void RdgBuilder::build_pass_batches()
{
	RDG_LOG("Building pass batches----------------------------------------------------------------------------");
	pass_batches_.clear();
	// Step 1: Build the dependency graph
	std::unordered_map<std::string, std::vector<int>> resource_to_producers;
	std::vector<std::unordered_set<int>>              adjacency_list(rdg_passes_.size());
	std::vector<int>                                  indegree(rdg_passes_.size(), 0);

	for (int i = 0; i < rdg_passes_.size(); ++i)
	{
		const PassInfo &pass_info = rdg_passes_[i]->get_pass_info();
		for (const auto &output : pass_info.outputs)
		{
			resource_to_producers[output.name].push_back(i);
		}
	}

	for (int i = 0; i < rdg_passes_.size(); ++i)
	{
		const PassInfo &pass_info = rdg_passes_[i]->get_pass_info();
		for (const auto &input : pass_info.inputs)
		{
			if (resource_to_producers.contains(input.name))
			{
				for (int producer : resource_to_producers[input.name])
				{
					if (adjacency_list[producer].insert(i).second)
					{
						indegree[i]++;
					}
				}
			}
		}
	}
	// Step 2: Perform Kahn's Algorithm for topological sorting and build pass batches
	std::unordered_map<std::string, ResourceState> resource_states;

	std::queue<int> zero_indegree_queue;
	for (int i = 0; i < indegree.size(); ++i)
	{
		if (indegree[i] == 0)
		{
			zero_indegree_queue.push(i);
		}
	}

	uint32_t    processed_node_count = 0;
	RdgPassType current_batch_type   = RdgPassType::kNone;
	PassBatch   current_batch;

	while (!zero_indegree_queue.empty())
	{
		int node = zero_indegree_queue.front();
		zero_indegree_queue.pop();
		++processed_node_count;

		RdgPass    *current_pass = rdg_passes_[node].get();
		current_pass->reset_before_frame();
		RdgPassType pass_type    = current_pass->get_pass_type();

		// Start a new batch if the pass type changes
		if (pass_type != current_batch_type)
		{
			if (!current_batch.passes.empty())
			{
				// Finish the current batch
				pass_batches_.push_back(std::move(current_batch));
				current_batch = PassBatch{};
			}
			current_batch_type = pass_type;
			current_batch.type = pass_type;
		}

		// Add the current pass to the batch
		current_batch.passes.push_back(current_pass);
		current_pass->set_batch_index(pass_batches_.size());

		int64_t wait_batch_index = -1;

		const PassInfo &pass_info = current_pass->get_pass_info();

		// Process inputs
		for (uint32_t i = 0; i < pass_info.inputs.size(); ++i)
		{
			const auto        &input         = pass_info.inputs[i];
			const std::string &resource_name = input.name;

			RDG_LOG("  Processing Input [{}]: {}", i, resource_name.c_str());

			if (resource_states.contains(resource_name))
			{
				ResourceState &state = resource_states[resource_name];

				if (state.prev_pass != -1)
				{
					auto prev_pass       = rdg_passes_[state.prev_pass].get();
					if (prev_pass->get_pass_type() != current_pass->get_pass_type())
					{
						// Need to wait on the semaphore from the last write pass
						wait_batch_index = std::max(wait_batch_index, prev_pass->get_batch_index());
					}
					common::ImageMemoryBarrier barrier{};
					if(setup_memory_barrier(state, current_pass, barrier))
					{
						current_pass->add_input_memory_barrier(i, barrier);
						state.prev_access_mask = barrier.dst_access_mask;
						state.prev_layout      = barrier.new_layout;
						state.prev_stage_mask  = barrier.dst_stage_mask;
						state.prev_pass = node;	
					}
				}
			}
			else
			{
				// Initialize resource state if not present
				resource_states[resource_name] = ResourceState{};
			}

			// Set input image view if necessary
			current_pass->set_input_image_view(i, resource_states[resource_name].image_view);
		}

		// Update the batch's wait semaphore value
		current_batch.wait_batch_index = std::max(current_batch.wait_batch_index, wait_batch_index);

		// Process outputs
		for (uint32_t i = 0; i < pass_info.outputs.size(); ++i)
		{
			const auto        &output        = pass_info.outputs[i];
			const std::string &resource_name = output.name;

			ResourceState &state = resource_states[resource_name];

			state.producer_pass = node;
			state.prev_pass     = node;

			// Update resource state with new layout, stage mask, access mask
			if (output.type == kAttachment)
			{
				if (common::is_depth_format(output.format))
				{
					state.prev_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					state.prev_stage_mask  = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
					state.prev_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
				}
				else
				{
					state.prev_layout      = vk::ImageLayout::eColorAttachmentOptimal;
					state.prev_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
					state.prev_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
				}

				if (current_pass->get_pass_type() == RdgPassType::kCompute)
				{
					state.prev_layout     = vk::ImageLayout::eShaderReadOnlyOptimal;
					state.prev_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
					state.prev_access_mask = vk::AccessFlagBits2::eNone;
				}

				common::ImageMemoryBarrier barrier{};
				barrier.old_layout      = vk::ImageLayout::eUndefined;
				barrier.new_layout      = state.prev_layout;
				barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
				barrier.dst_stage_mask  = state.prev_stage_mask;
				barrier.src_access_mask = {};
				barrier.dst_access_mask = state.prev_access_mask;

				current_pass->add_output_memory_barrier(i, barrier);
			}
			else if (output.type == kSwapchain)
			{
				state.prev_layout      = vk::ImageLayout::eColorAttachmentOptimal;
				state.prev_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
				state.prev_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;

				common::ImageMemoryBarrier barrier{};
				barrier.old_layout      = vk::ImageLayout::eUndefined;
				barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
				barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
				barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
				barrier.src_access_mask = {};
				barrier.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;

				current_pass->add_output_memory_barrier(i, barrier);
			}
			else
			{
				throw std::runtime_error("Not Implemented");
			}

			auto &image_view = render_context_.get_active_frame().get_render_target(rdg_passes_[node]->get_name()).get_views();
			state.image_view = &image_view[i];
		}

		// Process adjacent nodes
		for (int neighbor : adjacency_list[node])
		{
			indegree[neighbor]--;
			if (indegree[neighbor] == 0)
			{
				zero_indegree_queue.push(neighbor);
			}
		}
	}

	// Finish the last batch if any passes are left
	if (!current_batch.passes.empty())
	{
		pass_batches_.push_back(std::move(current_batch));
	}

	// Check for cycles (if the processed node count is not equal to the number of nodes)
	if (processed_node_count != rdg_passes_.size())
	{
		throw std::runtime_error("Cycle detected in the pass dependency graph.");
	}

	RDG_LOG("End Building pass batches----------------------------------------------------------------------------");
}
}        // namespace xihe::rendering