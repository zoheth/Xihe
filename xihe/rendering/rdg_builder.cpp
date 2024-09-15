#include "rdg_builder.h"

namespace xihe::rendering
{

struct ResourceState
{
	int last_write_pass = -1;

	/*vk::PipelineStageFlags2 prev_stage_mask = vk::PipelineStageFlagBits2::eTopOfPipe;
	vk::AccessFlags2        prev_access_mask = vk::AccessFlagBits2::eNone;
	vk::ImageLayout         prev_layout      = vk::ImageLayout::eUndefined;*/
	vk::PipelineStageFlags2 producer_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
	vk::AccessFlags2        producer_access_mask = vk::AccessFlagBits2::eNone;
	vk::ImageLayout         producer_layout      = vk::ImageLayout::eUndefined;

	const backend::ImageView *image_view = nullptr;

	std::vector<int> read_passes;
};

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

	rdg_passes_.push_back(std::make_unique<RasterRdgPass>(name, render_context_, RdgPassType::kRaster, std::move(pass_info), std::move(subpasses)));

	render_context_.register_rdg_render_target(name, rdg_passes_.back().get());
}

void RdgBuilder::add_compute_pass(const std::string &name, PassInfo &&pass_info, const std::vector<backend::ShaderSource> &shader_sources, ComputeRdgPass::ComputeFunction &&compute_function)
{
	rdg_passes_.push_back(std::make_unique<ComputeRdgPass>(name, render_context_, RdgPassType::kCompute, std::move(pass_info), shader_sources));
	dynamic_cast<ComputeRdgPass *> (rdg_passes_.back().get())->set_compute_function(std::move(compute_function));

	render_context_.register_rdg_render_target(name, rdg_passes_.back().get());
}

void RdgBuilder::compile()
{
	/*topological_sort();
	setup_pass_dependencies();*/
	build_pass_batches();
}

void RdgBuilder::execute()
{
	compile();
#if 1
	size_t batch_count = pass_batches_.size();
	for (size_t i = 0; i < batch_count; ++i)
	{
		const auto       &batch          = pass_batches_[i];

		backend::CommandBuffer *command_buffer = nullptr;

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

		// Submit the command buffer with appropriate semaphores
		if (batch.type == RdgPassType::kRaster)
		{
			render_context_.graphics_submit(
			    {command_buffer},        // list of command buffers
			    batch.wait_semaphore_value,
			    batch.signal_semaphore_value,
			    is_first_submission,
			    is_last_submission);
		}
		else if (batch.type == RdgPassType::kCompute)
		{
			render_context_.compute_submit(
			    {command_buffer},        // list of command buffers
			    batch.wait_semaphore_value,
			    batch.signal_semaphore_value);
		}
	}
//	backend::CommandBuffer &graphics_command_buffer = render_context_.request_graphics_command_buffer(backend::CommandBuffer::ResetMode::kResetPool,
//	                                                                                         vk::CommandBufferLevel::ePrimary, 0);
//	backend::CommandBuffer &compute_command_buffer  = render_context_.request_compute_command_buffer(backend::CommandBuffer::ResetMode::kResetPool, vk::CommandBufferLevel::ePrimary, 0);
//
//	graphics_command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
//	compute_command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
//
//	auto pre_pass_type = rdg_passes_[pass_order_[0]]->get_pass_type();
//#if 1
//	for (auto &index : pass_order_)
//	{
//		auto         &rdg_pass      = rdg_passes_.at(index);
//		RenderTarget *render_target = rdg_pass->get_render_target();
//
//		if (rdg_pass->get_pass_type() != pre_pass_type)
//		{
//
//			if (rdg_pass->get_pass_type() == kCompute)
//			{
//				graphics_command_buffer.end();
//				render_context_.graphics_submit({&graphics_command_buffer}, rdg_pass->get_wait_semaphore_value(), rdg_pass->get_signal_semaphore_value());
//				graphics_command_buffer.reset(backend::CommandBuffer::ResetMode::kResetPool);
//				graphics_command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
//
//			}
//			else if (rdg_pass->get_pass_type() == kRaster)
//			{
//				compute_command_buffer.end();
//				render_context_.compute_submit({&compute_command_buffer}, rdg_pass->get_wait_semaphore_value(), rdg_pass->get_signal_semaphore_value());
//				compute_command_buffer.reset(backend::CommandBuffer::ResetMode::kResetPool);
//				compute_command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
//			}
//		}
//		if (rdg_pass->get_pass_type() == kCompute)
//		{
//			rdg_pass->execute(compute_command_buffer, *render_target, {});
//		}
//		else if (rdg_pass->get_pass_type() == kRaster)
//		{
//			set_viewport_and_scissor(graphics_command_buffer, render_target->get_extent());
//
//			rdg_pass->execute(graphics_command_buffer, *render_target, {});
//		}
//		pre_pass_type = rdg_pass->get_pass_type();
//	}
//
//	graphics_command_buffer.end();
//
//	render_context_.submit(graphics_command_buffer);
#elif 1

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
			auto *raster_pass = dynamic_cast<RasterRdgPass *>(rdg_pass.get());
			rdg_pass_tasks[index]      = std::vector<SecondaryDrawTask>(raster_pass->get_subpass_count());

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

// #define RDG_LOG_ENABLED

#ifdef RDG_LOG_ENABLED
#	define RDG_LOG(...) LOGI(__VA_ARGS__)
#else
#	define RDG_LOG(...)
#endif

void RdgBuilder::build_pass_batches()
{
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

	uint64_t graphics_semaphore_value = 0;
	uint64_t compute_semaphore_value  = 0;

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
		RdgPassType pass_type    = current_pass->get_pass_type();

		// Start a new batch if the pass type changes
		if (pass_type != current_batch_type)
		{
			if (!current_batch.passes.empty())
			{
				// Finish the current batch
				RDG_LOG("Finished PassBatch:");
				RDG_LOG("  Type: {}", (current_batch.type == RdgPassType::kRaster) ? "Raster" : "Compute");
				RDG_LOG("  Wait Semaphore Value: {}", current_batch.wait_semaphore_value);
				RDG_LOG("  Signal Semaphore Value: {}", current_batch.signal_semaphore_value);
				RDG_LOG("  Passes in Batch:");
				for (const auto &pass : current_batch.passes)
				{
					RDG_LOG("    Pass Name: {}", pass->get_name().c_str());
				}
				pass_batches_.push_back(std::move(current_batch));
				current_batch = PassBatch{};
			}
			current_batch_type = pass_type;
			current_batch.type = pass_type;

		}

		 // Add the current pass to the batch
		current_batch.passes.push_back(current_pass);

		RDG_LOG("");
		RDG_LOG("Processing Pass: {}", current_pass->get_name().c_str());
		RDG_LOG("  Pass Type: {}", (pass_type == RdgPassType::kRaster) ? "Raster" : "Compute");

		uint64_t wait_semaphore_value = 0;

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

				if (state.last_write_pass != -1)
				{
					RdgPass *last_writer_pass = rdg_passes_[state.last_write_pass].get();

					if (current_pass->get_pass_type() != last_writer_pass->get_pass_type())
					{
						// Need to wait on the semaphore from the last write pass
						wait_semaphore_value = std::max(wait_semaphore_value, last_writer_pass->get_signal_semaphore_value());

					}

					// Add memory barrier between last write and current read
					common::ImageMemoryBarrier barrier{};
					barrier.old_layout      = state.producer_layout;
					barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
					barrier.src_stage_mask  = state.producer_stage_mask;
					barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;
					barrier.src_access_mask = state.producer_access_mask;
					barrier.dst_access_mask = vk::AccessFlagBits2::eShaderRead;

					if (current_pass->get_pass_type() == RdgPassType::kCompute)
					{
						barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
					}

					current_pass->add_input_memory_barrier(i, barrier);

					state.last_write_pass = -1;        // Reset last write pass
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
		current_batch.wait_semaphore_value = std::max(current_batch.wait_semaphore_value, wait_semaphore_value);

		// Process outputs
		for (uint32_t i = 0; i < pass_info.outputs.size(); ++i)
		{
			const auto        &output        = pass_info.outputs[i];
			const std::string &resource_name = output.name;

			ResourceState &state = resource_states[resource_name];

			state.last_write_pass = node;

			// Update resource state with new layout, stage mask, access mask
			if (output.type == kAttachment)
			{
				if (common::is_depth_format(output.format))
				{
					state.producer_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					state.producer_stage_mask  = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
					state.producer_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
				}
				else
				{
					state.producer_layout      = vk::ImageLayout::eColorAttachmentOptimal;
					state.producer_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
					state.producer_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
				}

				common::ImageMemoryBarrier barrier{};
				barrier.old_layout      = vk::ImageLayout::eUndefined;
				barrier.new_layout      = state.producer_layout;
				barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
				barrier.dst_stage_mask  = state.producer_stage_mask;
				barrier.src_access_mask = {};
				barrier.dst_access_mask = state.producer_access_mask;

				current_pass->add_output_memory_barrier(i, barrier);
			}
			else if (output.type == kSwapchain)
			{
				state.producer_layout      = vk::ImageLayout::eColorAttachmentOptimal;
				state.producer_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
				state.producer_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;

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

		// Set the pass's wait semaphore value
		current_pass->set_wait_semaphore(wait_semaphore_value);

		// Set the pass's signal semaphore value and update the batch's signal semaphore value
		uint64_t signal_semaphore_value = 0;
		if (pass_type == RdgPassType::kCompute)
		{
			signal_semaphore_value = ++compute_semaphore_value;
		}
		else if (pass_type == RdgPassType::kRaster)
		{
			signal_semaphore_value = ++graphics_semaphore_value;
		}
		current_pass->set_signal_semaphore(signal_semaphore_value);
		current_batch.signal_semaphore_value = signal_semaphore_value;

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
		RDG_LOG("Finished PassBatch:");
		RDG_LOG("  Type: {}", (current_batch.type == RdgPassType::kRaster) ? "Raster" : "Compute");
		RDG_LOG("  Wait Semaphore Value: {}", current_batch.wait_semaphore_value);
		RDG_LOG("  Signal Semaphore Value: {}", current_batch.signal_semaphore_value);
		RDG_LOG("  Passes in Batch:");
		for (const auto &pass : current_batch.passes)
		{
			RDG_LOG("    Pass Name: {}", pass->get_name().c_str());
		}
		pass_batches_.push_back(std::move(current_batch));
	}

	// Check for cycles (if the processed node count is not equal to the number of nodes)
	if (processed_node_count != rdg_passes_.size())
	{
		throw std::runtime_error("Cycle detected in the pass dependency graph.");
	}

}

void RdgBuilder::topological_sort()
{
	// pass_order_.clear();
	pass_batches_.clear();

	// Step 1: Build the graph
	std::unordered_map<std::string, std::vector<int>> resource_to_producers;
	std::vector<std::unordered_set<int>>              adjacency_list(rdg_passes_.size());
	std::vector<int>                                  indegree(rdg_passes_.size(), 0);

	 std::unordered_map<std::string, uint64_t> resource_timeline_values;

	for (int i = 0; i < rdg_passes_.size(); ++i)
	{
		const PassInfo &pass_info = rdg_passes_[i]->get_pass_info();
		for (const auto &output : pass_info.outputs)
		{
			resource_to_producers[output.name].push_back(i);
			resource_timeline_values[output.name] = 0;
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

	// Step 2: Perform Kahn's Algorithm for topological sorting
	std::queue<int> zero_indegree_queue;
	for (int i = 0; i < indegree.size(); ++i)
	{
		if (indegree[i] == 0)
		{
			zero_indegree_queue.push(i);
		}
	}

	uint64_t graphics_semaphore_value = 0;
	uint64_t compute_semaphore_value  = 0;

	std::vector<RdgPass *> current_passes;

	
	uint32_t processed_node_count = 0;

	while (!zero_indegree_queue.empty())
	{
		int node = zero_indegree_queue.front();
		zero_indegree_queue.pop();
		++processed_node_count;

		current_passes.push_back(rdg_passes_[node].get());
		auto pass_type = rdg_passes_[node]->get_pass_type();
		bool need_submit = false;
		uint64_t signal_semaphore_value = 0;
		for (int neighbor : adjacency_list[node])
		{
			if (rdg_passes_[neighbor]->get_pass_type() != pass_type)
			{
				// Increment the semaphore only when it's identified as needing to be submitted for the first time.
				if (!need_submit)
				{
					if (pass_type == RdgPassType::kCompute)
					{
						++compute_semaphore_value;
						signal_semaphore_value = compute_semaphore_value;
					}
					else if (pass_type == RdgPassType::kRaster)
					{
						++graphics_semaphore_value;
						signal_semaphore_value = graphics_semaphore_value;
					}
				}

				rdg_passes_[neighbor]->set_wait_semaphore(signal_semaphore_value);

				need_submit = true;
			}

			indegree[neighbor]--;
			if (indegree[neighbor] == 0)
			{
				zero_indegree_queue.push(neighbor);
			}
		}
	}

	// Check for cycles (if the sorted_indices size is not equal to the number of nodes)
	if (processed_node_count != rdg_passes_.size())
	{
		throw std::runtime_error("Cycle detected in the pass dependency graph.");
	}
}

void RdgBuilder::setup_pass_dependencies()
{
	//std::unordered_map<std::string, ResourceState> resource_states;
	//std::unordered_map<std::string, uint64_t>      resource_timeline_values;

	//uint64_t graphics_semaphore_value = 0;
	//uint64_t compute_semaphore_value  = 0;

	//for (int idx : pass_order_)
	//{
	//	const PassInfo &pass_info = rdg_passes_[idx]->get_pass_info();

	//	uint64_t wait_semaphore_value = 0;

	//	for (uint32_t i = 0; i < pass_info.inputs.size(); ++i)
	//	{
	//		auto &input = pass_info.inputs[i];
	//		const std::string &resource_name = input.name;

	//		if (resource_states.contains(resource_name))
	//		{
	//			ResourceState &state = resource_states[resource_name];

	//			if (state.last_write_pass != -1)
	//			{

	//				if (rdg_passes_[idx]->get_pass_type() != rdg_passes_[state.last_write_pass]->get_pass_type())
	//				{
	//					wait_semaphore_value = std::max(wait_semaphore_value, rdg_passes_[state.last_write_pass]->get_signal_semaphore_value());
	//				}

	//				switch (input.type)
	//				{
	//					case kAttachment:
	//					{
	//						common::ImageMemoryBarrier barrier{};
	//						barrier.old_layout      = state.producer_layout;
	//						barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
	//						barrier.src_stage_mask  = state.producer_stage_mask;
	//						barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;
	//						barrier.src_access_mask = state.producer_access_mask;
	//						barrier.dst_access_mask = vk::AccessFlagBits2::eShaderRead;

	//						if (rdg_passes_[idx]->get_pass_type() == RdgPassType::kCompute)
	//						{
	//							barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
	//						}

	//						rdg_passes_[idx]->add_input_memory_barrier(i, barrier);
	//						break;
	//					}

	//					case kReference:
	//					{
	//						break;
	//					}
	//					default:
	//					{
	//						throw std::runtime_error("Not Implemented");
	//					}
	//				}
	//				state.last_write_pass = -1;
	//			}
	//		}
	//		rdg_passes_[idx]->set_input_image_view(i, resource_states[resource_name].image_view);
	//	}

	//	rdg_passes_[idx]->set_wait_semaphore(wait_semaphore_value);

	//	// Signal semaphore after pass execution
	//	if (rdg_passes_[idx]->get_pass_type() == RdgPassType::kCompute)
	//	{
	//		rdg_passes_[idx]->set_signal_semaphore(++compute_semaphore_value);
	//		// todo
	//		// Temporarily ignore the output processing of compute passes
	//		continue;
	//	}
	//	else if (rdg_passes_[idx]->get_pass_type() == RdgPassType::kRaster)
	//	{
	//		rdg_passes_[idx]->set_signal_semaphore(++graphics_semaphore_value);
	//	}

	//	auto &image_view = render_context_.get_active_frame().get_render_target(rdg_passes_[idx]->get_name()).get_views();
	//	for (uint32_t i = 0; i < pass_info.outputs.size(); ++i)
	//	{
	//		auto &output = pass_info.outputs[i];

	//		const std::string &resource_name = output.name;
	//		ResourceState     &state         = resource_states[resource_name];

	//		state.last_write_pass = idx;

	//		switch (output.type)
	//		{
	//			case kAttachment:
	//			{
	//				if (common::is_depth_format(output.format))
	//				{
	//					state.producer_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	//					state.producer_stage_mask  = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
	//					state.producer_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
	//				}
	//				else
	//				{
	//					state.producer_layout      = vk::ImageLayout::eColorAttachmentOptimal;
	//					state.producer_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	//					state.producer_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
	//				}

	//				common::ImageMemoryBarrier barrier{};
	//				barrier.old_layout      = vk::ImageLayout::eUndefined;
	//				barrier.new_layout      = state.producer_layout;
	//				barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
	//				barrier.dst_stage_mask  = state.producer_stage_mask;
	//				barrier.src_access_mask = {};
	//				barrier.dst_access_mask = state.producer_access_mask;

	//				rdg_passes_.at(idx)->add_output_memory_barrier(i, barrier);

	//				break;
	//			}
	//			case kSwapchain:
	//			{
	//				common::ImageMemoryBarrier barrier{};
	//				barrier.old_layout      = vk::ImageLayout::eUndefined;
	//				barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
	//				barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
	//				barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	//				barrier.src_access_mask = {};
	//				barrier.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;

	//				rdg_passes_.at(idx)->add_output_memory_barrier(i, barrier);

	//				break;
	//			}
	//			default:
	//			{
	//				throw std::runtime_error("Not Implemented");
	//			}
	//		}

	//		state.last_write_pass = idx;
	//		state.image_view      = &image_view[i];
	//	}
	//}
}
}        // namespace xihe::rendering
