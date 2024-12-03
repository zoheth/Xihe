#include "graph_builder.h"

namespace xihe::rendering
{
void ResourceStateTracker::track_resource(PassNode &pass_node, int32_t pass_idx)
{
	auto &pass_info = pass_node.get_pass_info();
	for (uint32_t i = 0; i < pass_info.outputs.size(); ++i)
	{
		auto &output = pass_info.outputs[i];
		if (states_.contains(output.name))
		{
			throw std::runtime_error("Resource already exists");
		}
		states_[output.name].producer_pass = pass_idx;
		states_[output.name].last_user     = pass_idx;
		if (auto *render_target = pass_node.get_render_target())
		{
			auto &views                     = render_target->get_views();
			states_[output.name].image_view = &views[i];
		}

		update_resource_state(output.usage, pass_node.get_type(), states_[output.name].usage_state);
	}
}

ResourceStateTracker::State &ResourceStateTracker::get_state(const std::string &name)
{
	auto it = states_.find(name);
	if (it == states_.end())
	{
		throw std::runtime_error("Resource state not found");
	}
	return it->second;
}

GraphBuilder::PassBuilder::PassBuilder(GraphBuilder &graph_builder, std::string name, std::unique_ptr<RenderPass> &&render_pass) :
    graph_builder_(graph_builder), pass_name_(std::move(name)), render_pass_(std::move(render_pass))
{}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::inputs(std::initializer_list<PassInput> inputs)
{
	pass_info_.inputs = inputs;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::outputs(std::initializer_list<PassOutput> outputs)
{
	pass_info_.outputs = outputs;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::shader(std::initializer_list<std::string> file_names)
{
	render_pass_->set_shader(file_names);
	return *this;
}

void GraphBuilder::PassBuilder::finalize()
{
	graph_builder_.add_pass(pass_name_, std::move(pass_info_),
	                        std::move(render_pass_));
}

void GraphBuilder::build_pass_batches()
{
	ResourceStateTracker resource_state_tracker;
	PassBatchBuilder     batch_builder;

	auto [adjacency_list, indegree] = build_dependency_graph();

	// Topological sort and build pass batches
	std::queue<uint32_t> zero_indegree_queue;
	for (uint32_t i = 0; i < indegree.size(); ++i)
	{
		if (indegree[i] == 0)
		{
			zero_indegree_queue.push(i);
		}
	}

	uint32_t processed_count = 0;
	while (!zero_indegree_queue.empty())
	{
		uint32_t node = zero_indegree_queue.front();
		zero_indegree_queue.pop();
		++processed_count;

		PassNode &current_pass = render_graph_.pass_nodes_[node];

		process_pass_inputs(node, current_pass, resource_state_tracker, batch_builder);

		process_pass_outputs(node, current_pass, resource_state_tracker);

		for (uint32_t neighbor : adjacency_list[node])
		{
			if (--indegree[neighbor] == 0)
			{
				zero_indegree_queue.push(neighbor);
			}
		}
	}

	if (processed_count != render_graph_.pass_nodes_.size())
	{
		throw std::runtime_error("Cycle detected in the pass dependency graph.");
	}

	render_graph_.pass_batches_ = batch_builder.finalize();
}

std::pair<std::vector<std::unordered_set<uint32_t>>, std::vector<uint32_t>> GraphBuilder::build_dependency_graph() const
{
	auto &pass_nodes = render_graph_.pass_nodes_;

	std::vector<std::unordered_set<uint32_t>> adjacency_list(pass_nodes.size());

	std::vector<uint32_t> indegree(pass_nodes.size(), 0);

	std::unordered_map<std::string, std::vector<uint32_t>> output_to_producers;
	for (uint32_t i = 0; i < pass_nodes.size(); ++i)
	{
		for (const auto &output : pass_nodes[i].get_pass_info().outputs)
		{
			output_to_producers[output.name].push_back(i);
		}
	}

	for (uint32_t consumer = 0; consumer < pass_nodes.size(); ++consumer)
	{
		const auto &inputs = pass_nodes[consumer].get_pass_info().inputs;
		for (const auto &input : inputs)
		{
			if (auto it = output_to_producers.find(input.name); it != output_to_producers.end())
			{
				for (uint32_t producer : it->second)
				{
					adjacency_list[producer].insert(consumer);
					indegree[consumer]++;
				}
			}
		}
	}

	return {adjacency_list, indegree};
}

void GraphBuilder::process_pass_inputs(uint32_t node, PassNode &current_pass, ResourceStateTracker &resource_tracker, PassBatchBuilder &batch_builder)
{
	batch_builder.process_pass(&current_pass);

	const auto &pass_info        = current_pass.get_pass_info();
	int64_t     wait_batch_index = -1;

	for (uint32_t i = 0; i < pass_info.inputs.size(); ++i)
	{
		const auto        &input         = pass_info.inputs[i];
		const std::string &resource_name = input.name;

		auto &resource_state = resource_tracker.get_state(resource_name);

		if (resource_state.last_user != -1)
		{
			auto &prev_pass = render_graph_.pass_nodes_[resource_state.last_user];
			if (prev_pass.get_type() != current_pass.get_type())
			{
				batch_builder.set_batch_dependency(prev_pass.get_batch_index());
			}

			current_pass.add_input_info(i, barrier, resource_state.image_view);

			if (barrier.new_layout != barrier.old_layout ||
			    barrier.src_stage_mask != barrier.dst_stage_mask ||
			    barrier.src_access_mask != barrier.dst_access_mask)
			{
				current_pass.add_input_info(i, barrier);

				// 更新资源状态
				resource_state.layout      = barrier.new_layout;
				resource_state.stage_mask  = barrier.dst_stage_mask;
				resource_state.access_mask = barrier.dst_access_mask;
				resource_state.last_user   = node;
			}
		}
	}
}

void GraphBuilder::add_pass(const std::string &name, PassInfo &&pass_info, std::unique_ptr<RenderPass> &&render_pass)
{
	is_dirty_ = true;

	std::vector<backend::Image> images;

	bool use_swapchain_image = false;
	for (const auto &output : pass_info.outputs)
	{
		if (output.type == RenderResourceType::kSwapchain)
		{
			use_swapchain_image = true;
		}
	}
	if (use_swapchain_image)
	{
		RenderTarget::CreateFunc create_func = [this, &pass_info](backend::Image &&swapchain_image) {
			auto                       &device = swapchain_image.get_device();
			std::vector<backend::Image> images;
			vk::Extent3D                swapchain_image_extent = swapchain_image.get_extent();
			if (auto swapchain_count = std::ranges::count_if(pass_info.outputs, [](const auto &output) {
				    return output.type == RenderResourceType::kSwapchain;
			    });
			    swapchain_count > 1)
			{
				throw std::runtime_error("Multiple swapchain outputs are not supported.");
			}
			for (const auto &output : pass_info.outputs)
			{
				if (output.type == RenderResourceType::kSwapchain)
				{
					images.push_back(std::move(swapchain_image));
					continue;
				}
				vk::Extent3D extent = swapchain_image_extent;

				backend::ImageBuilder image_builder{extent};
				image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
				    .with_format(output.format);
				if (common::is_depth_format(output.format))
				{
					image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | output.usage);
				}
				else
				{
					image_builder.with_usage(vk::ImageUsageFlagBits::eColorAttachment | output.usage);
				}
				backend::Image image = image_builder.build(device);
				images.push_back(std::move(image));
			}
			return std::make_unique<RenderTarget>(std::move(images));
		};

		render_context_.set_render_target_function(create_func);
	}
	else
	{
		// todo
	}

	PassNode pass_node{name, std::move(pass_info), std::move(render_pass)};
	render_graph_.add_pass_node(std::move(pass_node));
}

void GraphBuilder::build()
{
	build_pass_batches();
	/*PassBatch pass_batch;
	pass_batch.type = PassType::kRaster;
	pass_batch.pass_nodes.push_back(&render_graph_.pass_nodes_[0]);
	common::ImageMemoryBarrier barrier{};
	barrier.old_layout      = vk::ImageLayout::eUndefined;
	barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
	barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
	barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	barrier.src_access_mask = {};
	barrier.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;

	render_graph_.pass_nodes_[0].add_output_memory_barrier(0, std::move(barrier));

	render_graph_.pass_batches_.push_back(std::move(pass_batch));*/
}

void GraphBuilder::PassBatchBuilder::process_pass(PassNode *pass)
{
	if (current_batch_.type != pass->get_type() && !current_batch_.pass_nodes.empty())
	{
		finalize_current_batch();
	}

	if (current_batch_.pass_nodes.empty())
	{
		current_batch_.type = pass->get_type();
	}

	current_batch_.pass_nodes.push_back(pass);
	pass->set_batch_index(batches_.size() - 1);
}

void GraphBuilder::PassBatchBuilder::set_batch_dependency(int64_t wait_batch_index)
{
	current_batch_.wait_batch_index =
	    std::max(current_batch_.wait_batch_index, wait_batch_index);
}

std::vector<PassBatch> GraphBuilder::PassBatchBuilder::finalize()
{
	if (!current_batch_.pass_nodes.empty())
	{
		finalize_current_batch();
	}
	return std::move(batches_);
}

void GraphBuilder::PassBatchBuilder::finalize_current_batch()
{
	batches_.push_back(std::move(current_batch_));
	current_batch_ = PassBatch{};
}
}        // namespace xihe::rendering
