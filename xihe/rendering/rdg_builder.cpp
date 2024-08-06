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
	topological_sort();
	prepare_memory_barriers();
}

void RdgBuilder::execute()
{
	compile();

	backend::CommandBuffer &command_buffer = render_context_.request_graphics_command_buffer(backend::CommandBuffer::ResetMode::kResetPool,
	                                                                                         vk::CommandBufferLevel::ePrimary, 0);

	command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

#if 1
	for (auto &index : pass_order_)
	{
		auto         &rdg_pass      = rdg_passes_.at(index);
		RenderTarget *render_target = rdg_pass->get_render_target();

		set_viewport_and_scissor(command_buffer, render_target->get_extent());

		rdg_pass->execute(command_buffer, *render_target, {});
	}
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
		// RenderTarget *render_target = rdg_pass->get_render_target();

		/*auto     image_infos                         = rdg_pass->get_descriptor_image_infos(*render_target);
		uint32_t first_bindless_descriptor_set_index = std::numeric_limits<uint32_t>::max();
		for (auto &image_info : image_infos)
		{
		    auto index = render_context_.get_bindless_descriptor_set()->update(image_info);

		    if (index < first_bindless_descriptor_set_index)
		    {
		        first_bindless_descriptor_set_index = index;
		    }
		}*/

		rdg_pass_tasks[index] = std::vector<SecondaryDrawTask>(rdg_pass->get_subpass_count());

		rdg_pass->prepare(command_buffer);

		for (uint32_t i = 0; i < rdg_pass->get_subpass_count(); ++i)
		{
			auto &secondary_command_buffer = render_context_.get_active_frame().request_command_buffer(queue,
			                                                                                           reset_mode,
			                                                                                           vk::CommandBufferLevel::eSecondary,
			                                                                                           thread_index);

			rdg_pass->set_thread_index(i, thread_index);

			rdg_pass_tasks[index][i].init(&secondary_command_buffer, rdg_pass.get(), i);

			scheduler.AddTaskSetToPipe(&rdg_pass_tasks[index][i]);
			thread_index++;
		}
	}

	uint32_t pass_index = 0;

	for (auto &index : pass_order_)
	{
		auto &rdg_pass = rdg_passes_.at(index);

		std::vector<backend::CommandBuffer *> secondary_command_buffers;
		for (uint32_t i = 0; i < rdg_pass->get_subpass_count(); ++i)
		{
			scheduler.WaitforTask(&rdg_pass_tasks[index][i]);
			secondary_command_buffers.push_back(rdg_pass_tasks[index][i].command_buffer);
		}

		rdg_pass->execute(command_buffer, *rdg_pass->get_render_target(), secondary_command_buffers);

		pass_index++;
	}

#endif

	command_buffer.end();

	render_context_.submit(command_buffer);
}

void RdgBuilder::topological_sort()
{
	pass_order_.clear();

	// Step 1: Build the graph
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

	// Step 2: Perform Kahn's Algorithm for topological sorting
	std::queue<int> zero_indegree_queue;
	for (int i = 0; i < indegree.size(); ++i)
	{
		if (indegree[i] == 0)
		{
			zero_indegree_queue.push(i);
		}
	}

	while (!zero_indegree_queue.empty())
	{
		int node = zero_indegree_queue.front();
		zero_indegree_queue.pop();
		pass_order_.push_back(node);

		for (int neighbor : adjacency_list[node])
		{
			indegree[neighbor]--;
			if (indegree[neighbor] == 0)
			{
				zero_indegree_queue.push(neighbor);
			}
		}
	}

	// Check for cycles (if the sorted_indices size is not equal to the number of nodes)
	if (pass_order_.size() != rdg_passes_.size())
	{
		throw std::runtime_error("Cycle detected in the pass dependency graph.");
	}
}

void RdgBuilder::prepare_memory_barriers()
{
	std::unordered_map<std::string, ResourceState> resource_states;

	for (int idx : pass_order_)
	{
		const PassInfo &pass_info = rdg_passes_[idx]->get_pass_info();

		for (uint32_t i = 0; i < pass_info.inputs.size(); ++i)
		{
			auto &input = pass_info.inputs[i];

			const std::string &resource_name = input.name;
			if (resource_states.contains(resource_name))
			{
				ResourceState &state = resource_states[resource_name];

				if (state.last_write_pass != -1)
				{
					switch (input.type)
					{
						case kAttachment:
						{
							common::ImageMemoryBarrier barrier{};
							barrier.old_layout      = state.producer_layout;
							barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
							barrier.src_stage_mask  = state.producer_stage_mask;
							barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;
							barrier.src_access_mask = state.producer_access_mask;
							barrier.dst_access_mask = vk::AccessFlagBits2::eShaderRead;

							if (rdg_passes_[idx]->get_pass_type() == RdgPassType::kCompute)
							{
								barrier.dst_stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
							}

							rdg_passes_[idx]->add_input_memory_barrier(i, barrier);
							break;
						}

						case kReference:
						{
							break;
						}
						default:
						{
							throw std::runtime_error("Not Implemented");
						}
					}
				}
			}
			rdg_passes_[idx]->set_input_image_view(i, resource_states[resource_name].image_view);
		}
		auto &image_view = render_context_.get_active_frame().get_render_target(rdg_passes_[idx]->get_name()).get_views();
		for (uint32_t i = 0; i < pass_info.outputs.size(); ++i)
		{
			auto &output = pass_info.outputs[i];

			const std::string &resource_name = output.name;
			ResourceState     &state         = resource_states[resource_name];

			state.last_write_pass = idx;

			switch (output.type)
			{
				case kAttachment:
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

					rdg_passes_.at(idx)->add_output_memory_barrier(i, barrier);

					break;
				}
				case kSwapchain:
				{
					common::ImageMemoryBarrier barrier{};
					barrier.old_layout      = vk::ImageLayout::eUndefined;
					barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
					barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
					barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
					barrier.src_access_mask = {};
					barrier.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;

					rdg_passes_.at(idx)->add_output_memory_barrier(i, barrier);

					break;
				}
				default:
				{
					throw std::runtime_error("Not Implemented");
				}
			}

			state.last_write_pass = idx;
			state.image_view      = &image_view[i];
		}
	}
}
}        // namespace xihe::rendering
