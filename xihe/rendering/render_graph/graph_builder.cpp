#include "graph_builder.h"

namespace xihe::rendering
{
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
	PassBatchBuilder batch_builder;
}

std::pair<std::vector<std::unordered_set<int>>, std::vector<int>> GraphBuilder::build_dependency_graph()
{
	auto &pass_nodes = render_graph_.pass_nodes_;

	std::vector<std::unordered_set<int>> adjacency_list(pass_nodes.size());

	std::vector<int> indegree(pass_nodes.size(), 0);


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

	PassNode pass_node{name, PassType::kRaster, std::move(pass_info), std::move(render_pass)};
	render_graph_.add_pass_node(std::move(pass_node));
}

void GraphBuilder::build()
{
	PassBatch pass_batch;
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

	render_graph_.pass_batches_.push_back(std::move(pass_batch));
}
}        // namespace xihe::rendering
