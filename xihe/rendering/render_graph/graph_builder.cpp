#include "graph_builder.h"

namespace xihe::rendering
{
ResourceStateTracker::State ResourceStateTracker::get_or_create_state(const std::string &name)
{
	if (!states_.contains(name))
	{
		states_[name] = {};
	}
	return states_[name];
}

void ResourceStateTracker::track_resource(const std::string &name, uint32_t node, const ResourceUsageState &state)
{
	if (!states_.contains(name))
	{
		throw std::runtime_error("Resource state not found.");
	}
	states_[name].usage_state = state;

	states_[name].last_user = node;
}

GraphBuilder::PassBuilder::PassBuilder(GraphBuilder &graph_builder, std::string name, std::unique_ptr<RenderPass> &&render_pass) :
    graph_builder_(graph_builder), pass_name_(std::move(name)), render_pass_(std::move(render_pass))
{}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::bindables(std::initializer_list<PassBindable> bindables)
{
	pass_info_.bindables = bindables;
	return *this;
}

GraphBuilder::PassBuilder & GraphBuilder::PassBuilder::attachments(std::initializer_list<PassAttachment> attachments)
{
	pass_info_.attachments = attachments;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::shader(std::initializer_list<std::string> file_names)
{
	render_pass_->set_shader(file_names);
	return *this;
}

GraphBuilder::PassBuilder & GraphBuilder::PassBuilder::present()
{
	is_present_ = true;
	return *this;
}

void GraphBuilder::PassBuilder::finalize()
{
	graph_builder_.add_pass(pass_name_, std::move(pass_info_),
	                        std::move(render_pass_), is_present_);
}

void GraphBuilder::add_pass(const std::string &name, PassInfo &&pass_info, std::unique_ptr<RenderPass> &&render_pass, bool is_present)
{
	is_dirty_ = true;

	PassNode pass_node{render_graph_, name, std::move(pass_info), std::move(render_pass)};

	if (is_present)
	{
		common::ImageMemoryBarrier barrier;
		barrier.old_layout      = vk::ImageLayout::eUndefined;
		barrier.new_layout      = vk::ImageLayout::eColorAttachmentOptimal;
		barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
		barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		barrier.src_access_mask = {};
		barrier.dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
		pass_node.add_attachment_memory_barrier(0, barrier);
	}

	render_graph_.add_pass_node(std::move(pass_node));
}

void GraphBuilder::create_resources()
{
	struct ResourceCreateInfo
	{
		bool                 is_buffer   = false;
		bool                 is_external = false;
		bool                 is_handled  = false;        // Track if resource has been created
		vk::BufferUsageFlags buffer_usage{};
		vk::ImageUsageFlags  image_usage{};
		vk::Format           format = vk::Format::eUndefined;
		vk::Extent3D         extent{};
	};

	// First: Collect all resource information
	std::unordered_map<std::string, ResourceCreateInfo> resource_infos;
	for (auto &pass : render_graph_.pass_nodes_)
	{
		auto &info = pass.get_pass_info();

		for (const auto &bindable : info.bindables)
		{
			auto &res_info = resource_infos[bindable.name];
			switch (bindable.type)
			{
				case BindableType::kSampled:
					res_info.image_usage |= vk::ImageUsageFlagBits::eSampled;
					break;
				case BindableType::kStorageRead:
					res_info.image_usage |= vk::ImageUsageFlagBits::eStorage;
					break;
				case BindableType::kStorageWrite:
				case BindableType::kStorageReadWrite:
					res_info.image_usage |= vk::ImageUsageFlagBits::eStorage;
					res_info.format = bindable.format;
				//todo
					res_info.extent = bindable.extent;
					break;
			}
		}

		// Collect attachment info
		for (const auto &attachment : info.attachments)
		{
			auto &res_info  = resource_infos[attachment.name];
			res_info.format = attachment.format;
			if (attachment.extent.width > 0)
			{
				res_info.extent = attachment.extent;
			}
			if (attachment.type == AttachmentType::kColor)
			{
				res_info.image_usage |= vk::ImageUsageFlagBits::eColorAttachment;
				if (res_info.format == vk::Format::eUndefined)
				{
					res_info.format = vk::Format::eR8G8B8A8Unorm;
				}
			}
			else if (attachment.type == AttachmentType::kDepth)
			{
				res_info.image_usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
				if (res_info.format == vk::Format::eUndefined)
				{
					res_info.format = vk::Format::eD32Sfloat;
				}
			}
		}
	}

	backend::Device &device = render_context_.get_device();

	// Second: Create RenderTargets for passes with attachments
	for (auto &pass : render_graph_.pass_nodes_)
	{
		auto &info = pass.get_pass_info();
		if (info.attachments.empty())
		{
			continue;
		}

		// Prepare images and resource infos together
		std::vector<backend::Image> rt_images;
		rt_images.reserve(info.attachments.size());
		std::vector<ResourceInfo> resource_infos_to_update;
		resource_infos_to_update.reserve(info.attachments.size());

		for (const auto &attachment : info.attachments)
		{
			auto &res_info = resource_infos[attachment.name];

			vk::Extent3D extent = res_info.extent;
			// Create image
			if (extent == vk::Extent3D{})
			{
				extent.height = render_context_.get_swapchain().get_extent().height;
				extent.width  = render_context_.get_swapchain().get_extent().width;
				extent.depth  = 1;
			}
			backend::ImageBuilder image_builder{extent};
			image_builder.with_format(res_info.format)
			    .with_usage(res_info.image_usage)
			    .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);
			rt_images.push_back(image_builder.build(device));

			// Prepare resource info (except image_view which we'll set later)
			ResourceInfo resource_info;
			resource_info.external = res_info.is_external;
			ResourceInfo::ImageDesc image_desc;
			image_desc.format  = res_info.format;
			image_desc.extent  = extent;
			image_desc.usage   = res_info.image_usage;
			resource_info.desc = image_desc;
			resource_infos_to_update.push_back(resource_info);

			res_info.is_handled = true;
		}

		// Create RenderTarget
		auto render_target = std::make_unique<RenderTarget>(std::move(rt_images));

		// Update resource infos with image views
		for (size_t i = 0; i < info.attachments.size(); ++i)
		{
			std::get_if<ResourceInfo::ImageDesc>(&resource_infos_to_update[i].desc)->image_view = &render_target->get_views()[i];
			render_graph_.resources_[info.attachments[i].name] = resource_infos_to_update[i];
		}

		pass.set_render_target(std::move(render_target));
	}

	// Third: Create remaining resources
	for (const auto &[name, info] : resource_infos)
	{
		if (info.is_handled)
		{
			continue;
		}

		ResourceInfo resource_info;
		resource_info.external = info.is_external;

		if (info.is_buffer)
		{
			ResourceInfo::BufferDesc buffer_desc;
			buffer_desc.usage = info.buffer_usage;
			// todo
			resource_info.desc = buffer_desc;
		}
		else
		{
			ResourceInfo::ImageDesc image_desc;
			image_desc.format = info.format;
			// todo
			image_desc.extent = info.extent;
			image_desc.usage  = info.image_usage;

			backend::ImageBuilder image_builder{image_desc.extent};
			image_builder.with_format(info.format)
			    .with_usage(info.image_usage)
			    .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

			render_graph_.images_.push_back(image_builder.build_unique(device));
			render_graph_.image_views_.emplace_back(std::make_unique<backend::ImageView>(*render_graph_.images_.back(), vk::ImageViewType::e2D));
			image_desc.image_view = render_graph_.image_views_.back().get();
			resource_info.desc    = image_desc;
		}

		render_graph_.resources_[name] = resource_info;
	}
}

void GraphBuilder::build_pass_batches()
{
	ResourceStateTracker resource_state_tracker;
	PassBatchBuilder     batch_builder;

	auto [adjacency_list, indegree] = build_dependency_graph();

	create_resources();

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

		batch_builder.process_pass(&current_pass);

		process_pass_resources(node, current_pass, resource_state_tracker, batch_builder);

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
	auto                                     &pass_nodes = render_graph_.pass_nodes_;
	std::vector<std::unordered_set<uint32_t>> adjacency_list(pass_nodes.size());
	std::vector<uint32_t>                     indegree(pass_nodes.size(), 0);

	// 记录所有写入操作的producer
	std::unordered_map<std::string, std::vector<uint32_t>> resource_writers;

	// 收集所有写操作
	for (uint32_t i = 0; i < pass_nodes.size(); ++i)
	{
		const auto &pass_info = pass_nodes[i].get_pass_info();

		// 收集bindable中的写操作
		for (const auto &resource : pass_info.bindables)
		{
			if (resource.type == BindableType::kStorageBufferReadWrite ||
			    resource.type == BindableType::kStorageBufferWrite ||
			    resource.type == BindableType::kStorageReadWrite ||
			    resource.type == BindableType::kStorageWrite)
			{
				resource_writers[resource.name].push_back(i);
			}
		}

		// attachments总是写操作
		for (const auto &attachment : pass_info.attachments)
		{
			resource_writers[attachment.name].push_back(i);
		}
	}

	// 建立依赖
	for (uint32_t consumer = 0; consumer < pass_nodes.size(); ++consumer)
	{
		const auto &pass_info = pass_nodes[consumer].get_pass_info();

		// 检查bindable的依赖
		for (const auto &resource : pass_info.bindables)
		{
			if (auto it = resource_writers.find(resource.name);
			    it != resource_writers.end())
			{
				for (uint32_t producer : resource_writers[resource.name])
				{
					/*adjacency_list[producer].insert(consumer);
					indegree[consumer]++;*/
					if (producer == consumer)
					{
						continue;
					}

					if (adjacency_list[producer].insert(consumer).second)
					{
						indegree[consumer]++;
					}
				}
			}
		}
	}

	return {adjacency_list, indegree};
}

void GraphBuilder::process_pass_resources(uint32_t node, PassNode &pass, ResourceStateTracker &tracker, PassBatchBuilder &batch_builder)
{
	const PassInfo &pass_info = pass.get_pass_info();

	for (size_t i = 0; i < pass_info.bindables.size(); ++i)
	{
		const auto &bindable = pass_info.bindables[i];

		ResourceUsageState new_state;
		update_bindable_state(bindable.type, pass.get_type(), new_state);

		auto state = tracker.get_or_create_state(bindable.name);

		// todo buffer barrier

		common::ImageMemoryBarrier barrier;
		barrier.src_stage_mask  = state.usage_state.stage_mask;
		barrier.src_access_mask = state.usage_state.access_mask;
		barrier.dst_stage_mask  = new_state.stage_mask;
		barrier.dst_access_mask = new_state.access_mask;

		barrier.old_layout = state.usage_state.layout;
		barrier.new_layout = new_state.layout;

		if (state.last_user != -1 && render_graph_.pass_nodes_[state.last_user].get_type() != pass.get_type())
		{
			common::ImageMemoryBarrier release_barrier;

			auto &prev_pass = render_graph_.pass_nodes_[state.last_user];
			batch_builder.set_batch_dependency(render_graph_.pass_nodes_[state.last_user].get_batch_index());


			if (pass.get_type() == PassType::kCompute)
			{
				release_barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);
				release_barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);

				barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);
				barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);
			}
			else if (pass.get_type() == PassType::kRaster)
			{
				release_barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);
				release_barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);

				barrier.old_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eCompute);
				barrier.new_queue_family = render_context_.get_queue_family_index(vk::QueueFlagBits::eGraphics);
			}
			release_barrier.old_layout = state.usage_state.layout;
			release_barrier.src_stage_mask  = state.usage_state.stage_mask;
			release_barrier.src_access_mask = state.usage_state.access_mask;

			release_barrier.new_layout      = new_state.layout;
			release_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eBottomOfPipe;
			release_barrier.dst_access_mask = vk::AccessFlagBits2::eNone;

			prev_pass.add_release_barrier(bindable.name, release_barrier);

			// barrier.old_layout      = new_state.layout;
			barrier.src_stage_mask  = new_state.stage_mask;
			barrier.src_access_mask = vk::AccessFlagBits2::eNone;
		}

		pass.add_bindable(i, bindable.name, barrier);	
		
		tracker.track_resource(bindable.name, node, new_state);
	}

	for (size_t i = 0; i < pass_info.attachments.size(); ++i)
	{
		const auto        &attachment = pass_info.attachments[i];
		ResourceUsageState new_state;
		update_attachment_state(attachment.type, new_state);
		auto                       state = tracker.get_or_create_state(attachment.name);
		common::ImageMemoryBarrier barrier;
		barrier.src_stage_mask  = state.usage_state.stage_mask;
		barrier.src_access_mask = state.usage_state.access_mask;
		barrier.dst_stage_mask  = new_state.stage_mask;
		barrier.dst_access_mask = new_state.access_mask;
		barrier.old_layout      = state.usage_state.layout;
		barrier.new_layout      = new_state.layout;
		tracker.track_resource(attachment.name, node, new_state);
		pass.add_attachment_memory_barrier(i,std::move(barrier));
	}
}

void GraphBuilder::build()
{
	if (is_dirty_)
	{
		build_pass_batches();	
	}
	is_dirty_ = false;
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
	pass->set_batch_index(batches_.size());
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
