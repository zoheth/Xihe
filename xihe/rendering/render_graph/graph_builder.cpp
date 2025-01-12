#include "graph_builder.h"

namespace xihe::rendering
{
ResourceStateTracker::State ResourceStateTracker::get_or_create_state(const ResourceHandle &handle)
{
	if (!states_.contains(handle))
	{
		states_[handle] = {};
	}
	return states_[handle];
}

void ResourceStateTracker::track_resource(const ResourceHandle &handle, uint32_t node, const ResourceUsageState &state)
{
	if (!states_.contains(handle))
	{
		throw std::runtime_error("Resource state not found.");
	}
	if (handle.base_layer > 0)
	{
		// If only a portion is written, then the entire should wait for this portion to complete writing
		ResourceHandle new_handle       = handle;
		new_handle.base_layer           = 0;
		new_handle.layer_count          = handle.base_layer + 1;
		states_[new_handle].usage_state = state;
		states_[new_handle].last_user   = node;
	}
	states_[handle].usage_state = state;

	states_[handle].last_user = node;
}

GraphBuilder::PassBuilder::PassBuilder(GraphBuilder &graph_builder, std::string name, std::unique_ptr<RenderPass> &&render_pass) :
    graph_builder_(graph_builder), pass_name_(std::move(name)), render_pass_(std::move(render_pass))
{}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::bindables(std::initializer_list<PassBindable> bindables)
{
	pass_info_.bindables = bindables;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::attachments(std::initializer_list<PassAttachment> attachments)
{
	pass_info_.attachments = attachments;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::shader(std::initializer_list<std::string> file_names)
{
	render_pass_->set_shader(file_names);
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::bindables(const std::vector<PassBindable> &bindables)
{
	pass_info_.bindables = bindables;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::attachments(const std::vector<PassAttachment> &attachments)
{
	pass_info_.attachments = attachments;
	return *this;
}

GraphBuilder::PassBuilder & GraphBuilder::PassBuilder::copy(uint32_t attachment_index, std::unique_ptr<backend::ImageView> &&dst_image_view, const vk::ImageCopy &copy_region)
{
	image_read_back_ = std::make_unique<PassNode::ImageCopyInfo>();
	image_read_back_->attachment_index = attachment_index;
	image_read_back_->image_view       = std::move(dst_image_view);
	image_read_back_->copy_region      = copy_region;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::present()
{
	is_present_ = true;
	return *this;
}

GraphBuilder::PassBuilder &GraphBuilder::PassBuilder::gui(Gui *gui)
{
	gui_ = gui;
	return *this;
}

void GraphBuilder::PassBuilder::finalize()
{
	graph_builder_.add_pass(pass_name_, std::move(pass_info_),
	                        std::move(render_pass_), is_present_, std::move(image_read_back_), gui_);
}

void GraphBuilder::add_pass(const std::string &name, PassInfo &&pass_info, std::unique_ptr<RenderPass> &&render_pass, bool is_present, std::unique_ptr<PassNode::ImageCopyInfo> &&image_read_back, Gui * gui)
{
	is_dirty_ = true;

	PassNode pass_node{render_graph_, name, std::move(pass_info), std::move(render_pass)};

	if (gui)
	{
		pass_node.set_gui(gui);
	}

	if (image_read_back)
	{
		pass_node.set_image_copy_info(std::move(image_read_back));
	}


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
	collect_resource_create_info();

	create_graph_resource();
}

void GraphBuilder::collect_resource_create_info()
{
	resource_create_infos_.clear();

	for (auto &pass : render_graph_.pass_nodes_)
	{
		auto &info = pass.get_pass_info();

		for (auto &bindable : info.bindables)
		{
			auto &res_info = resource_create_infos_[bindable.name];
			switch (bindable.type)
			{
				case BindableType::kSampled:
					res_info.image_usage |= vk::ImageUsageFlagBits::eSampled;
					break;
				case BindableType::kSampledCube:
					res_info.image_usage |= vk::ImageUsageFlagBits::eSampled;
					res_info.image_flags |= vk::ImageCreateFlagBits::eCubeCompatible;
					break;
				case BindableType::kStorageRead:
					res_info.image_usage |= vk::ImageUsageFlagBits::eStorage;
					break;
				case BindableType::kStorageWrite:
				case BindableType::kStorageReadWrite:
					res_info.image_usage |= vk::ImageUsageFlagBits::eStorage;
					res_info.format      = bindable.format;
					res_info.extent_desc = bindable.extent_desc;
					break;
				case BindableType::kStorageBufferRead:
				case BindableType::kStorageBufferWrite:
				case BindableType::kStorageBufferReadWrite:
					res_info.is_buffer = true;
					res_info.buffer_usage |= vk::BufferUsageFlagBits::eStorageBuffer;
					res_info.buffer_size = std::max(res_info.buffer_size, bindable.buffer_size);
					break;
				case BindableType::kStorageBufferWriteClear:
					res_info.is_buffer = true;
					res_info.buffer_usage |= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
					res_info.buffer_size = std::max(res_info.buffer_size, bindable.buffer_size);
					break;
				case BindableType::kIndirectBuffer:
					res_info.is_buffer = true;
					res_info.buffer_usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
					break;
			}
			res_info.array_layers = std::max(res_info.array_layers, bindable.image_properties.array_layers);
		}

		// Collect attachment info
		for (const auto &attachment : info.attachments)
		{
			auto &res_info       = resource_create_infos_[attachment.name];
			res_info.format      = attachment.format;
			res_info.extent_desc = attachment.extent_desc;

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
			res_info.is_external = attachment.is_external;
			if (attachment.is_external)
			{
				res_info.image_usage |= vk::ImageUsageFlagBits::eTransferSrc;
			}
			res_info.array_layers = std::max(res_info.array_layers, attachment.image_properties.array_layers);
		}
	}
}

void GraphBuilder::create_graph_resource()
{
	vk::Extent2D swapchain_extent = render_context_.get_swapchain().get_extent();

	backend::Device                                  &device = render_context_.get_device();
	std::unordered_map<std::string, backend::Image *> base_images;
	for (const auto &[name, info] : resource_create_infos_)
	{
		ResourceInfo resource_info;
		resource_info.external = info.is_external;

		if (info.is_buffer)
		{
			backend::BufferBuilder buffer_builder{info.buffer_size};
			buffer_builder.with_usage(info.buffer_usage)
			    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);
			render_graph_.buffers_.push_back(buffer_builder.build_unique(device));
			render_graph_.buffers_.back()->set_debug_name(name);
			ResourceHandle handle{
			    .name = name};

			ResourceInfo::BufferDesc buffer_desc;
			buffer_desc.usage  = info.buffer_usage;
			buffer_desc.buffer = render_graph_.buffers_.back().get();

			resource_info.desc = buffer_desc;

			render_graph_.resources_[handle] = resource_info;
		}
		else
		{
			ResourceInfo::ImageDesc image_desc;
			image_desc.format = info.format;
			// todo
			image_desc.extent = info.extent_desc.calculate(swapchain_extent);
			image_desc.usage  = info.image_usage;

			vk::Extent3D extent = image_desc.extent;
			if (extent == vk::Extent3D{})
			{
				extent = vk::Extent3D{
				    render_context_.get_swapchain().get_extent().width,
				    render_context_.get_swapchain().get_extent().height,
				    1};
			}

			backend::ImageBuilder image_builder{extent};
			image_builder.with_format(info.format)
			    .with_usage(info.image_usage)
			    .with_array_layers(info.array_layers)
				.with_flags(info.image_flags)
			    .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

			render_graph_.images_.push_back(image_builder.build_unique(device));
			base_images[name] = render_graph_.images_.back().get();
		}
	}

	// Third: Create image views
	for (auto &pass : render_graph_.pass_nodes_)
	{
		auto                           &info = pass.get_pass_info();
		std::vector<backend::ImageView> rt_image_views;
		for (auto &attachment : info.attachments)
		{
			auto &res_info = resource_create_infos_[attachment.name];
			if (res_info.is_handled || res_info.is_buffer)
			{
				continue;
			}
			backend::Image *image = base_images[attachment.name];
			if (!image)
			{
				throw std::runtime_error("Image not found.");
			}
			if (attachment.image_properties.n_use_layer == 0)
			{
				attachment.image_properties.n_use_layer = res_info.array_layers;
			}
			auto view_type = attachment.image_properties.n_use_layer > 1 ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
			rt_image_views.emplace_back(*image, view_type, res_info.format, 0, attachment.image_properties.current_layer, 0, attachment.image_properties.n_use_layer);
		}
		if (!rt_image_views.empty())
		{
			auto render_target = std::make_unique<RenderTarget>(std::move(rt_image_views));
			pass.set_render_target(std::move(render_target));
		}

		for (auto &bindable : info.bindables)
		{
			auto       *base_image = base_images[bindable.name];
			const auto &res_info   = resource_create_infos_[bindable.name];

			if (res_info.is_buffer)
			{
				continue;
			}
			if (bindable.image_properties.n_use_layer == 0)
			{
				bindable.image_properties.n_use_layer = res_info.array_layers;
			}

			ResourceHandle handle{
			    .name        = bindable.name,
			    .base_layer  = bindable.image_properties.current_layer,
			    .layer_count = bindable.image_properties.n_use_layer};

			if (render_graph_.resources_.contains(handle))
			{
				continue;
			}

			vk::ImageViewType view_type = bindable.image_properties.n_use_layer > 1 ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
			if (bindable.type == BindableType::kSampledCube)
			{
				// todo
				view_type = vk::ImageViewType::eCubeArray;
			}

			auto image_view = std::make_unique<backend::ImageView>(*base_image, view_type, res_info.format, 0, handle.base_layer, 0, bindable.image_properties.n_use_layer);

			ResourceInfo resource_info;
			resource_info.external = res_info.is_external;
			ResourceInfo::ImageDesc image_desc;
			image_desc.format     = res_info.format;
			image_desc.extent     = res_info.extent_desc.calculate(swapchain_extent);
			image_desc.usage      = res_info.image_usage;
			image_desc.image_view = image_view.get();
			resource_info.desc    = image_desc;

			render_graph_.resources_[handle] = resource_info;
			render_graph_.image_views_.push_back(std::move(image_view));
		}
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

	// Process resource dependencies
	std::unordered_map<std::string, std::vector<uint32_t>> resource_writers;

	for (uint32_t i = 0; i < pass_nodes.size(); ++i)
	{
		const auto &pass_info = pass_nodes[i].get_pass_info();

		for (const auto &resource : pass_info.bindables)
		{
			if (resource.type == BindableType::kStorageBufferReadWrite ||
			    resource.type == BindableType::kStorageBufferWrite ||
			    resource.type == BindableType::kStorageBufferWriteClear ||
			    resource.type == BindableType::kStorageReadWrite ||
			    resource.type == BindableType::kStorageWrite)
			{
				resource_writers[resource.name].push_back(i);
			}
		}

		// attachments always write
		for (const auto &attachment : pass_info.attachments)
		{
			resource_writers[attachment.name].push_back(i);
		}
	}

	for (uint32_t consumer = 0; consumer < pass_nodes.size(); ++consumer)
	{
		const auto &pass_info = pass_nodes[consumer].get_pass_info();

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

		ResourceHandle handle{
		    .name        = bindable.name,
		    .base_layer  = bindable.image_properties.current_layer,
		    .layer_count = bindable.image_properties.n_use_layer};

		auto state = tracker.get_or_create_state(handle);

		typedef common::BufferMemoryBarrier MemoryBarrierBase;

		MemoryBarrierBase barrier;
		barrier.src_access_mask = state.usage_state.access_mask;
		barrier.dst_access_mask = new_state.access_mask;
		barrier.src_stage_mask  = state.usage_state.stage_mask;
		barrier.dst_stage_mask  = new_state.stage_mask;

		if (state.last_user != -1 && render_graph_.pass_nodes_[state.last_user].get_type() != pass.get_type())
		{
			batch_builder.set_batch_dependency(render_graph_.pass_nodes_[state.last_user].get_batch_index());
			auto &prev_pass = render_graph_.pass_nodes_[state.last_user];

			MemoryBarrierBase release_barrier;
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

			release_barrier.src_stage_mask  = state.usage_state.stage_mask;
			release_barrier.src_access_mask = state.usage_state.access_mask;

			/* release_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eBottomOfPipe;
			 release_barrier.dst_access_mask = vk::AccessFlagBits2::eNone;*/
			release_barrier.dst_stage_mask  = new_state.stage_mask;
			release_barrier.dst_access_mask = new_state.access_mask;

			if (is_buffer(bindable.type))
			{
				common::BufferMemoryBarrier buffer_barrier;
				buffer_barrier.src_access_mask  = release_barrier.src_access_mask;
				buffer_barrier.dst_access_mask  = release_barrier.dst_access_mask;
				buffer_barrier.src_stage_mask   = release_barrier.src_stage_mask;
				buffer_barrier.dst_stage_mask   = release_barrier.dst_stage_mask;
				buffer_barrier.old_queue_family = release_barrier.old_queue_family;
				buffer_barrier.new_queue_family = release_barrier.new_queue_family;
				prev_pass.add_release_barrier(handle, buffer_barrier);
			}
			else
			{
				common::ImageMemoryBarrier image_barrier;
				image_barrier.src_access_mask  = release_barrier.src_access_mask;
				image_barrier.dst_access_mask  = release_barrier.dst_access_mask;
				image_barrier.src_stage_mask   = release_barrier.src_stage_mask;
				image_barrier.dst_stage_mask   = release_barrier.dst_stage_mask;
				image_barrier.old_queue_family = release_barrier.old_queue_family;
				image_barrier.new_queue_family = release_barrier.new_queue_family;

				image_barrier.old_layout = state.usage_state.layout;
				image_barrier.new_layout = new_state.layout;

				prev_pass.add_release_barrier(handle, image_barrier);
			}

			// barrier.src_stage_mask  = new_state.stage_mask;
			// barrier.src_access_mask = vk::AccessFlagBits2::eNone;
		}

		if (is_buffer(bindable.type))
		{
			common::BufferMemoryBarrier buffer_barrier;
			buffer_barrier.src_access_mask  = barrier.src_access_mask;
			buffer_barrier.dst_access_mask  = barrier.dst_access_mask;
			buffer_barrier.src_stage_mask   = barrier.src_stage_mask;
			buffer_barrier.dst_stage_mask   = barrier.dst_stage_mask;
			buffer_barrier.old_queue_family = barrier.old_queue_family;
			buffer_barrier.new_queue_family = barrier.new_queue_family;
			pass.add_bindable(i, handle, buffer_barrier);
		}
		else
		{
			common::ImageMemoryBarrier image_barrier;
			image_barrier.src_access_mask  = barrier.src_access_mask;
			image_barrier.dst_access_mask  = barrier.dst_access_mask;
			image_barrier.src_stage_mask   = barrier.src_stage_mask;
			image_barrier.dst_stage_mask   = barrier.dst_stage_mask;
			image_barrier.old_queue_family = barrier.old_queue_family;
			image_barrier.new_queue_family = barrier.new_queue_family;
			image_barrier.old_layout       = state.usage_state.layout;
			image_barrier.new_layout       = new_state.layout;
			pass.add_bindable(i, handle, image_barrier);
		}

		tracker.track_resource(handle, node, new_state);
	}

	for (size_t i = 0; i < pass_info.attachments.size(); ++i)
	{
		const auto &attachment = pass_info.attachments[i];

		ResourceHandle handle{
		    .name        = attachment.name,
		    .base_layer  = attachment.image_properties.current_layer,
		    .layer_count = attachment.image_properties.n_use_layer};
		ResourceUsageState new_state;
		update_attachment_state(attachment.type, new_state);
		auto                       state = tracker.get_or_create_state(handle);
		common::ImageMemoryBarrier barrier;
		barrier.src_stage_mask  = state.usage_state.stage_mask;
		barrier.src_access_mask = state.usage_state.access_mask;
		barrier.dst_stage_mask  = new_state.stage_mask;
		barrier.dst_access_mask = new_state.access_mask;
		barrier.old_layout      = state.usage_state.layout;
		barrier.new_layout      = new_state.layout;
		tracker.track_resource(handle, node, new_state);
		pass.add_attachment_memory_barrier(i, barrier);
	}
}

GraphBuilder::GraphBuilder(RenderGraph &render_graph, RenderContext &render_context) :
    render_graph_(render_graph), render_context_(render_context)
{
	render_context.set_on_surface_change([this]() {
		recreate_resources();
	});
}

void GraphBuilder::build()
{
	if (is_dirty_)
	{
		build_pass_batches();
	}
	is_dirty_ = false;
}

void GraphBuilder::recreate_resources()
{
	render_graph_.image_views_.clear();
	render_graph_.images_.clear();
	render_graph_.resources_.clear();

	create_graph_resource();
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
