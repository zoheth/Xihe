#include "rdg_pass.h"

#include "render_context.h"
#include "rendering/subpass.h"

namespace xihe::rendering
{
RdgPass::RdgPass(std::string name, RenderContext &render_context, const RdgPassType pass_type, PassInfo &&pass_info) :
    name_{std::move(name)}, render_context_{render_context}, pass_type_(pass_type), pass_info_{std::move(pass_info)}
{
	for (const auto &output : pass_info_.outputs)
	{
		if (output.type == RdgResourceType::kSwapchain)
		{
			use_swapchain_image_ = true;
		}
		if (!output.override_resolution.width)
		{
			needs_recreate_rt_ = true;
		}
		load_store_.emplace_back(output.load_op, vk::AttachmentStoreOp::eStore);
		if (output.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
		{
			load_store_.back().store_op = vk::AttachmentStoreOp::eDontCare;
		}
		if (common::is_depth_format(output.format))
		{
			clear_value_.emplace_back(vk::ClearDepthStencilValue{0.0f, 0});
		}
		else
		{
			clear_value_.emplace_back(vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f});
		}
	}

	create_render_target_func_ = [this](backend::Image &&swapchain_image) {
		auto &device = swapchain_image.get_device();

		std::vector<backend::Image> images;

		vk::Extent3D swapchain_image_extent = swapchain_image.get_extent();

		if (auto swapchain_count = std::ranges::count_if(pass_info_.outputs, [](const auto &output) {
			    return output.type == RdgResourceType::kSwapchain;
		    });
		    swapchain_count > 1)
		{
			throw std::runtime_error("Multiple swapchain outputs are not supported.");
		}

		for (const auto &output : pass_info_.outputs)
		{
			if (output.type == RdgResourceType::kSwapchain)
			{
				images.push_back(std::move(swapchain_image));
				continue;
			}

			vk::Extent3D extent = swapchain_image_extent;
			if (output.override_resolution.width != 0 && output.override_resolution.height != 0)
			{
				extent = vk::Extent3D(output.override_resolution.width, output.override_resolution.height, 1);
			}
			if (output.modify_extent)
			{
				extent = output.modify_extent(extent);
			}

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

	input_image_views_.resize(pass_info_.inputs.size());
}

std::unique_ptr<RenderTarget> RdgPass::create_render_target(backend::Image &&swapchain_image) const
{
	return create_render_target_func_(std::move(swapchain_image));
}

RenderTarget *RdgPass::get_render_target() const
{
	return &render_context_.get_active_frame().get_render_target(name_);
	/*if (needs_recreate_rt())
	{
	    return &render_context_.get_active_frame().get_render_target(name_);
	}
	assert(render_target_ && "If use_swapchain_image returns false, the render_target_ must be created during initialization.");
	return render_target_.get();*/
}

std::vector<vk::DescriptorImageInfo> RdgPass::get_descriptor_image_infos(RenderTarget &render_target) const
{
	auto &views = render_target.get_views();

	std::vector<vk::DescriptorImageInfo> descriptor_image_infos{};

	for (uint32_t i = 0; i < pass_info_.outputs.size(); ++i)
	{
		if ((pass_info_.outputs[i].usage & vk::ImageUsageFlagBits::eSampled) && pass_info_.outputs[i].sampler)
		{
			vk::DescriptorImageInfo descriptor_image_info{};
			descriptor_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			descriptor_image_info.imageView   = views[i].get_handle();
			descriptor_image_info.sampler     = pass_info_.outputs[i].sampler->get_handle();

			descriptor_image_infos.push_back(descriptor_image_info);
		}
	}

	return descriptor_image_infos;
}

void RdgPass::prepare(backend::CommandBuffer &command_buffer)
{
}

backend::Device &RdgPass::get_device() const
{
	return render_context_.get_device();
}

const RdgPassType &RdgPass::get_pass_type() const
{
	return pass_type_;
}

const std::string &RdgPass::get_name() const
{
	return name_;
}

PassInfo &RdgPass::get_pass_info()
{
	return pass_info_;
}

bool RdgPass::needs_recreate_rt() const
{
	return needs_recreate_rt_;
}

void RdgPass::set_batch_index(int64_t index)
{
	batch_index_ = index;
}

int64_t RdgPass::get_batch_index() const
{
	if (batch_index_ == -1)
	{
		throw std::runtime_error("Batch index not set.");
	}
	return batch_index_;
}

void RdgPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents)
{
	for (const auto &[index, barrier] : input_barriers_)
	{
		if (std::holds_alternative<common::ImageMemoryBarrier>(barrier))
		{
			command_buffer.image_memory_barrier(*input_image_views_[index], std::get<common::ImageMemoryBarrier>(barrier));
		}
		else
		{
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
}

void RdgPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
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

void RdgPass::set_input_image_view(uint32_t index, const backend::ImageView *image_view)
{
	assert(index < input_image_views_.size());
	input_image_views_[index] = image_view;
}

std::vector<const backend::ImageView *> &RdgPass::get_input_image_views()
{
	return input_image_views_;
}

void RdgPass::add_input_memory_barrier(uint32_t index, Barrier &&barrier)
{
	input_barriers_[index] = barrier;
}

void RdgPass::add_output_memory_barrier(uint32_t index, Barrier &&barrier)
{
	output_barriers_[index] = barrier;
}

void RdgPass::reset_before_frame()
{
	release_barriers_.clear();
}

void RdgPass::add_release_barrier(const backend::ImageView *image_view, Barrier &&barrier)
{
	release_barriers_[image_view] = barrier;
}

RasterRdgPass::RasterRdgPass(std::string name, RenderContext &render_context, PassInfo &&pass_info, std::vector<std::unique_ptr<Subpass>> &&subpasses) :
    RdgPass(std::move(name), render_context, kRaster, std::move(pass_info)), subpasses_{std::move(subpasses)}
{
	for (auto &subpass : subpasses_)
	{
		subpass->prepare();
	}
}

void RasterRdgPass::prepare(backend::CommandBuffer &command_buffer)
{
	render_pass_ = &command_buffer.get_render_pass(*get_render_target(), load_store_, subpasses_);
	framebuffer_ = &get_device().get_resource_cache().request_framebuffer(*get_render_target(), *render_pass_);
}

void RasterRdgPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers)
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

std::vector<std::unique_ptr<Subpass>> &RasterRdgPass::get_subpasses()
{
	return subpasses_;
}

uint32_t RasterRdgPass::get_subpass_count() const
{
	return subpasses_.size();
}

const std::vector<common::LoadStoreInfo> &RasterRdgPass::get_load_store() const
{
	return load_store_;
}

backend::RenderPass &RasterRdgPass::get_render_pass() const
{
	return *render_pass_;
}

backend::Framebuffer &RasterRdgPass::get_framebuffer() const
{
	return *framebuffer_;
}

void RasterRdgPass::set_thread_index(uint32_t subpass_index, uint32_t thread_index)
{
	assert(subpass_index < subpasses_.size());
	subpasses_[subpass_index]->set_thread_index(thread_index);
}

void RasterRdgPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents)
{
	RdgPass::begin_draw(command_buffer, render_target, contents);

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

void RasterRdgPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	command_buffer.end_render_pass();

	if (use_swapchain_image_)
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

	RdgPass::end_draw(command_buffer, render_target);
}

void RasterRdgPass::add_subpass(std::unique_ptr<Subpass> &&subpass)
{
	subpass->prepare();
	subpasses_.emplace_back(std::move(subpass));
}

ComputeRdgPass::ComputeRdgPass(std::string name, RenderContext &render_context, PassInfo &&pass_info, const std::vector<backend::ShaderSource> &shader_sources) :
    RdgPass(std::move(name), render_context, kCompute, std::move(pass_info))
{
	for (const auto &shader_source : shader_sources)
	{
		auto &shader_module = get_device().get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eCompute, shader_source);
		pipeline_layouts_.push_back(&get_device().get_resource_cache().request_pipeline_layout({&shader_module}));
	}
}

void ComputeRdgPass::set_compute_function(ComputeFunction &&compute_function)
{
	compute_function_ = std::move(compute_function);
}

void ComputeRdgPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers)
{
	begin_draw(command_buffer, render_target, vk::SubpassContents::eInline);
	compute_function_(command_buffer, *this);
	end_draw(command_buffer, render_target);
}

std::vector<backend::PipelineLayout *> &ComputeRdgPass::get_pipeline_layouts()
{
	return pipeline_layouts_;
}

void RasterRdgPass::draw_subpass(backend::CommandBuffer &command_buffer, const RenderTarget &render_target, uint32_t i) const
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
