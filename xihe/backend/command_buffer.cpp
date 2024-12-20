#include "command_buffer.h"

#include "backend/command_pool.h"
#include "backend/device.h"
#include "rendering/render_frame.h"
#include "vulkan/vulkan_format_traits.hpp"

namespace xihe::backend
{
CommandBuffer::CommandBuffer(CommandPool &command_pool, vk::CommandBufferLevel level) :
    VulkanResource(nullptr, &command_pool.get_device()),
    level_{level},
    command_pool_{command_pool},
    max_push_constants_size_{get_device().get_gpu().get_properties().limits.maxPushConstantsSize}

{
	vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(), level, 1);

	set_handle(get_device().get_handle().allocateCommandBuffers(allocate_info).front());
}

CommandBuffer::CommandBuffer(CommandBuffer &&other) noexcept :
    VulkanResource(std::move(other)),
    level_{other.level_},
    command_pool_{other.command_pool_},
    max_push_constants_size_{other.max_push_constants_size_}
{}

CommandBuffer::~CommandBuffer()
{
	if (get_handle())
	{
		get_device().get_handle().freeCommandBuffers(command_pool_.get_handle(), get_handle());
	}
}

vk::Result CommandBuffer::begin(vk::CommandBufferUsageFlags flags, CommandBuffer *primary_cmd_buf)
{
	if (level_ == vk::CommandBufferLevel::eSecondary)
	{
		// todo
	}
	pipeline_state_.reset();
	resource_binding_state_.reset();
	descriptor_set_layout_binding_state_.clear();
	stored_push_constants_.clear();
	vk::CommandBufferBeginInfo begin_info(flags);
	get_handle().begin(begin_info);
	return vk::Result::eSuccess;
}


void CommandBuffer::begin_rendering(rendering::RenderTarget &render_target, const std::vector<vk::ClearValue> &clear_values)
{
	pipeline_state_.reset();

	color_attachments_.clear();
	depth_attachment_ = std::nullopt;

	AttachmentsState attachments_state;

	for (size_t i = 0; i < render_target.get_views().size(); i++)
	{
		if (common::is_depth_format(render_target.get_views()[i].get_format()))
		{
			depth_attachment_ = vk::RenderingAttachmentInfo(
			    render_target.get_views()[i].get_handle(),
			    vk::ImageLayout::eDepthStencilAttachmentOptimal,
			    {},
			    {},
			    {},
			    vk::AttachmentLoadOp::eClear,
			    vk::AttachmentStoreOp::eStore,
			    i < clear_values.size() ? clear_values[i] : vk::ClearValue{});
			attachments_state.depth_attachment_format = render_target.get_views()[i].get_format();
		}
		else
		{
			color_attachments_.push_back(vk::RenderingAttachmentInfo(
			    render_target.get_views()[i].get_handle(),
			    vk::ImageLayout::eColorAttachmentOptimal,
			    {},
			    {},
			    {},
			    vk::AttachmentLoadOp::eClear,
			    vk::AttachmentStoreOp::eStore,
			    i < clear_values.size() ? clear_values[i] : vk::ClearValue{}));

			attachments_state.color_attachment_formats.push_back(render_target.get_views()[i].get_format());
		}
	}

	set_attachments_state(attachments_state);

	auto blend_state = pipeline_state_.get_color_blend_state();
	blend_state.attachments.resize(color_attachments_.size());
	set_color_blend_state(blend_state);

	vk::RenderingAttachmentInfo *p_depth_attachment = nullptr;
	if (depth_attachment_.has_value())
	{
		p_depth_attachment = &depth_attachment_.value();
	}

	vk::RenderingInfo rendering_info(
	    {},                                                     // flags
	    {{}, render_target.get_extent()},                       // renderArea
	    1,                                                      // layerCount
	    0,                                                      // viewMask
	    static_cast<uint32_t>(color_attachments_.size()),        // colorAttachmentCount
	    color_attachments_.data(),                               // pColorAttachments
	    p_depth_attachment,                                      // pDepthAttachment
	    nullptr                                                 // pStencilAttachment
	);

	get_handle().beginRendering(rendering_info);
}

void CommandBuffer::execute_commands(CommandBuffer &secondary_command_buffer)
{
	get_handle().executeCommands(secondary_command_buffer.get_handle());
}

void CommandBuffer::execute_commands(std::vector<CommandBuffer *> &secondary_command_buffers)
{
	std::vector<vk::CommandBuffer> command_buffers(secondary_command_buffers.size());

	std::ranges::transform(secondary_command_buffers, command_buffers.begin(), [](CommandBuffer *command_buffer) { return command_buffer->get_handle(); });

	get_handle().executeCommands(command_buffers);
}

void CommandBuffer::end_rendering()
{
	get_handle().endRendering();
}

void CommandBuffer::set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data)
{
	pipeline_state_.set_specialization_constant(constant_id, data);
}

void CommandBuffer::push_constants(const std::vector<uint8_t> &values)
{
	uint32_t size = to_u32(stored_push_constants_.size() + values.size());

	if (size > max_push_constants_size_)
	{
		LOGE("Push constant limit of {} exceeded (pushing {} bytes for a total of {} bytes)", max_push_constants_size_, values.size(), size);
		throw std::runtime_error("Cannot overflow push constant limit");
	}

	stored_push_constants_.insert(stored_push_constants_.end(), values.begin(), values.end());
}

void CommandBuffer::bind_buffer(const backend::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state_.bind_buffer(buffer, offset, range, set, binding, array_element);
}

void CommandBuffer::bind_image(const backend::ImageView &image_view, const backend::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state_.bind_image(image_view, sampler, set, binding, array_element);
}

void CommandBuffer::bind_image(const backend::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state_.bind_image(image_view, set, binding, array_element);
}

void CommandBuffer::bind_input(const backend::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
	resource_binding_state_.bind_input(image_view, set, binding, array_element);
}

void CommandBuffer::bind_index_buffer(const backend::Buffer &buffer, vk::DeviceSize offset, vk::IndexType index_type)
{
	get_handle().bindIndexBuffer(buffer.get_handle(), offset, index_type);
}

vk::Result CommandBuffer::end()
{
	get_handle().end();
	return vk::Result::eSuccess;
}

void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
	flush(vk::PipelineBindPoint::eGraphics);
	get_handle().draw(vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
	flush(vk::PipelineBindPoint::eGraphics);

	get_handle().drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::draw_indexed_indirect(const backend::Buffer &buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride)
{
	flush(vk::PipelineBindPoint::eGraphics);

	get_handle().drawIndexedIndirect(buffer.get_handle(), offset, draw_count, stride);
}

void CommandBuffer::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
	flush(vk::PipelineBindPoint::eCompute);

	get_handle().dispatch(group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::dispatch_indirect(const backend::Buffer &buffer, vk::DeviceSize offset)
{
	flush(vk::PipelineBindPoint::eCompute);

	get_handle().dispatchIndirect(buffer.get_handle(), offset);
}

void CommandBuffer::draw_mesh_tasks(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
	flush(vk::PipelineBindPoint::eGraphics);

	get_handle().drawMeshTasksEXT(group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::draw_mesh_tasks_indirect(const backend::Buffer &buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride)
{
	flush(vk::PipelineBindPoint::eGraphics);
	get_handle().drawMeshTasksIndirectEXT(buffer.get_handle(), offset, draw_count, stride);
}

void CommandBuffer::draw_mesh_tasks_indirect_count(const backend::Buffer &buffer, vk::DeviceSize offset, const backend::Buffer &count_buffer, vk::DeviceSize count_buffer_offset, uint32_t max_draw_count, uint32_t stride)
{
	flush(vk::PipelineBindPoint::eGraphics);
	get_handle().drawMeshTasksIndirectCountEXT(buffer.get_handle(), offset, count_buffer.get_handle(), count_buffer_offset, max_draw_count, stride);
}

void CommandBuffer::update_buffer(const backend::Buffer &buffer, vk::DeviceSize offset, const std::vector<uint8_t> &data)
{
	get_handle().updateBuffer(buffer.get_handle(), offset, data.size(), data.data());
}

void CommandBuffer::blit_image(const backend::Image &src_img, const backend::Image &dst_img, const std::vector<vk::ImageBlit> &regions)
{
	get_handle().blitImage(src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions, vk::Filter::eLinear);
}

void CommandBuffer::resolve_image(const backend::Image &src_img, const backend::Image &dst_img, const std::vector<vk::ImageResolve> &regions)
{
	get_handle().resolveImage(src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
}

void CommandBuffer::copy_buffer(const backend::Buffer &src_buffer, const backend::Buffer &dst_buffer, vk::DeviceSize size)
{
	get_handle().copyBuffer(src_buffer.get_handle(), dst_buffer.get_handle(), vk::BufferCopy(0, 0, size));
}

void CommandBuffer::copy_image(const backend::Image &src_img, const backend::Image &dst_img, const std::vector<vk::ImageCopy> &regions)
{
	get_handle().copyImage(src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
}

void CommandBuffer::copy_buffer_to_image(const backend::Buffer &buffer, const backend::Image &image, const std::vector<vk::BufferImageCopy> &regions)
{
	get_handle().copyBufferToImage(buffer.get_handle(), image.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
}

void CommandBuffer::copy_image_to_buffer(const backend::Image &image, vk::ImageLayout image_layout, const backend::Buffer &buffer, const std::vector<vk::BufferImageCopy> &regions)
{
	get_handle().copyImageToBuffer(image.get_handle(), image_layout, buffer.get_handle(), regions);
}

void CommandBuffer::image_memory_barrier(const backend::ImageView &image_view, const common::ImageMemoryBarrier &memory_barrier) const
{
	auto subresource_range = image_view.get_subresource_range();
	auto format            = image_view.get_format();

	// Adjust the aspect mask if the format is a depth format
	if (common::is_depth_only_format(format))
	{
		subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
	}
	else if (common::is_depth_stencil_format(format))
	{
		subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	}

	common::image_layout_transition(get_handle(), image_view.get_image().get_handle(), memory_barrier, subresource_range);
}

void CommandBuffer::buffer_memory_barrier(const backend::Buffer &buffer, vk::DeviceSize offset, vk::DeviceSize size, const common::BufferMemoryBarrier &memory_barrier)
{
	vk::BufferMemoryBarrier2 buffer_memory_barrier{
	    {},
	    memory_barrier.src_access_mask,
	    {},
	    memory_barrier.dst_access_mask,
	    {},
	    {},
	    buffer.get_handle(),
	    offset,
	    size};
	vk::DependencyInfo dependency_info{};
	dependency_info.setBufferMemoryBarriers({buffer_memory_barrier});
	get_handle().pipelineBarrier2(dependency_info);
}

void CommandBuffer::set_update_after_bind(bool update_after_bind)
{
	update_after_bind_ = update_after_bind;
}

void CommandBuffer::bind_vertex_buffers(uint32_t first_binding, const std::vector<std::reference_wrapper<const backend::Buffer>> &buffers, const std::vector<vk::DeviceSize> &offsets)
{
	std::vector<vk::Buffer> buffer_handles(buffers.size(), nullptr);

	std::ranges::transform(buffers, buffer_handles.begin(), [](const backend::Buffer &buffer) { return buffer.get_handle(); });

	get_handle().bindVertexBuffers(first_binding, buffer_handles, offsets);
}

vk::Result CommandBuffer::reset(ResetMode reset_mode)
{
	assert(reset_mode == command_pool_.get_reset_mode() && "Command buffer reset mode must match the one used by the pool to allocate it");

	if (reset_mode == ResetMode::kResetIndividually)
	{
		get_handle().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	}

	return vk::Result::eSuccess;
}

void CommandBuffer::set_viewport_state(const ViewportState &state_info)
{
	pipeline_state_.set_viewport_state(state_info);
}

void CommandBuffer::set_attachments_state(const AttachmentsState &state_info)
{
	pipeline_state_.set_attachments_state(state_info);
}

void CommandBuffer::set_vertex_input_state(const VertexInputState &state_info)
{
	pipeline_state_.set_vertex_input_state(state_info);
}

void CommandBuffer::set_input_assembly_state(const InputAssemblyState &state_info)
{
	pipeline_state_.set_input_assembly_state(state_info);
}

void CommandBuffer::set_rasterization_state(const RasterizationState &state_info)
{
	pipeline_state_.set_rasterization_state(state_info);
}

void CommandBuffer::set_multisample_state(const MultisampleState &state_info)
{
	pipeline_state_.set_multisample_state(state_info);
}

void CommandBuffer::set_depth_stencil_state(const DepthStencilState &state_info)
{
	pipeline_state_.set_depth_stencil_state(state_info);
}

void CommandBuffer::set_color_blend_state(const ColorBlendState &state_info)
{
	pipeline_state_.set_color_blend_state(state_info);
}

void CommandBuffer::set_viewport(uint32_t first_viewport, const std::vector<vk::Viewport> &viewports)
{
	get_handle().setViewport(first_viewport, viewports);
}

void CommandBuffer::set_scissor(uint32_t first_scissor, const std::vector<vk::Rect2D> &scissors)
{
	get_handle().setScissor(first_scissor, scissors);
}

void CommandBuffer::set_line_width(float line_width)
{
	get_handle().setLineWidth(line_width);
}

void CommandBuffer::set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor)
{
	get_handle().setDepthBias(depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
}

void CommandBuffer::set_blend_constants(const std::array<float, 4> &blend_constants)
{
	get_handle().setBlendConstants(blend_constants.data());
}

void CommandBuffer::set_depth_bounds(float min_depth_bounds, float max_depth_bounds)
{
	get_handle().setDepthBounds(min_depth_bounds, max_depth_bounds);
}

void CommandBuffer::set_has_mesh_shader(bool has_mesh_shader)
{
	pipeline_state_.set_has_mesh_shader(has_mesh_shader);
}

void CommandBuffer::bind_pipeline_layout(PipelineLayout &pipeline_layout)
{
	pipeline_state_.set_pipeline_layout(pipeline_layout);
}

void CommandBuffer::flush(vk::PipelineBindPoint pipeline_bind_point)
{
	flush_pipeline_state(pipeline_bind_point);
	flush_push_constants();
	flush_descriptor_state(pipeline_bind_point);
}

void CommandBuffer::flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point)
{
	assert(command_pool_.get_render_frame() && "The command pool must be associated to a render frame");

	const auto &pipeline_layout = pipeline_state_.get_pipeline_layout();

	std::unordered_set<uint32_t> update_descriptor_sets;

	// Iterate over the shader sets to check if they have already been bound
	// If they have, add the set so that the command buffer later updates it
	for (auto &set_it : pipeline_layout.get_shader_sets())
	{
		uint32_t descriptor_set_id = set_it.first;

		auto descriptor_set_layout_it = descriptor_set_layout_binding_state_.find(descriptor_set_id);

		if (descriptor_set_layout_it != descriptor_set_layout_binding_state_.end())
		{
			if (descriptor_set_layout_it->second->get_handle() != pipeline_layout.get_descriptor_set_layout(descriptor_set_id).get_handle())
			{
				update_descriptor_sets.emplace(descriptor_set_id);
			}
		}
	}

	// Validate that the bound descriptor set layouts exist in the pipeline layout
	for (auto set_it = descriptor_set_layout_binding_state_.begin(); set_it != descriptor_set_layout_binding_state_.end();)
	{
		if (!pipeline_layout.has_descriptor_set_layout(set_it->first))
		{
			set_it = descriptor_set_layout_binding_state_.erase(set_it);
		}
		else
		{
			++set_it;
		}
	}

	// Check if a descriptor set needs to be created
	if (resource_binding_state_.is_dirty() || !update_descriptor_sets.empty())
	{
		resource_binding_state_.clear_dirty();

		// Iterate over all of the resource sets bound by the command buffer
		for (auto &resource_set_it : resource_binding_state_.get_resource_sets())
		{
			uint32_t descriptor_set_id = resource_set_it.first;
			auto    &resource_set      = resource_set_it.second;

			// Don't update resource set if it's not in the update list OR its state hasn't changed
			if (!resource_set.is_dirty() && (!update_descriptor_sets.contains(descriptor_set_id)))
			{
				continue;
			}

			// Clear dirty flag for resource set
			resource_binding_state_.clear_dirty(descriptor_set_id);

			// Skip resource set if a descriptor set layout doesn't exist for it
			if (!pipeline_layout.has_descriptor_set_layout(descriptor_set_id))
			{
				continue;
			}

			auto &descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(descriptor_set_id);

			// Make descriptor set layout bound for current set
			descriptor_set_layout_binding_state_[descriptor_set_id] = &descriptor_set_layout;

			BindingMap<vk::DescriptorBufferInfo> buffer_infos;
			BindingMap<vk::DescriptorImageInfo>  image_infos;

			std::vector<uint32_t> dynamic_offsets;

			// Iterate over all resource bindings
			for (auto &binding_it : resource_set.get_resource_bindings())
			{
				auto  binding_index     = binding_it.first;
				auto &binding_resources = binding_it.second;

				// Check if binding exists in the pipeline layout
				if (auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
				{
					// Iterate over all binding resources
					for (auto &element_it : binding_resources)
					{
						auto  array_element = element_it.first;
						auto &resource_info = element_it.second;

						// Pointer references
						auto &buffer     = resource_info.buffer;
						auto &sampler    = resource_info.sampler;
						auto &image_view = resource_info.image_view;

						// Get buffer info
						if (buffer != nullptr && common::is_buffer_descriptor_type(binding_info->descriptorType))
						{
							vk::DescriptorBufferInfo buffer_info(resource_info.buffer->get_handle(), resource_info.offset, resource_info.range);

							if (common::is_dynamic_buffer_descriptor_type(binding_info->descriptorType))
							{
								dynamic_offsets.push_back(to_u32(buffer_info.offset));
								buffer_info.offset = 0;
							}

							buffer_infos[binding_index][array_element] = buffer_info;
						}

						// Get image info
						else if (image_view != nullptr || sampler != nullptr)
						{
							// Can be null for input attachments
							vk::DescriptorImageInfo image_info(sampler ? sampler->get_handle() : nullptr, image_view->get_handle());

							if (image_view != nullptr)
							{
								// Add image layout info based on descriptor type
								switch (binding_info->descriptorType)
								{
									case vk::DescriptorType::eCombinedImageSampler:
										image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
										break;
									case vk::DescriptorType::eInputAttachment:
										image_info.imageLayout =
										    common::is_depth_format(image_view->get_format()) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
										break;
									case vk::DescriptorType::eStorageImage:
										image_info.imageLayout = vk::ImageLayout::eGeneral;
										break;
									default:
										continue;
								}
							}

							image_infos[binding_index][array_element] = image_info;
						}
					}

					assert((!update_after_bind_ ||
					        (buffer_infos.contains(binding_index) || (image_infos.contains(binding_index)))) &&
					       "binding index with no buffer or image infos can't be checked for adding to bindings_to_update");
				}
			}

			vk::DescriptorSet descriptor_set_handle = command_pool_.get_render_frame()->request_descriptor_set(
			    descriptor_set_layout, buffer_infos, image_infos, update_after_bind_, command_pool_.get_thread_index());

			// Bind descriptor set
			get_handle().bindDescriptorSets(pipeline_bind_point, pipeline_layout.get_handle(), descriptor_set_id, descriptor_set_handle, dynamic_offsets);
		}
	}
	if (const auto bindless_descriptor_set = pipeline_layout.get_bindless_descriptor_set())
	{
		get_handle().bindDescriptorSets(pipeline_bind_point, pipeline_layout.get_handle(), pipeline_layout.get_bindless_descriptor_set_index(), bindless_descriptor_set, {});
	}
}

void CommandBuffer::flush_pipeline_state(vk::PipelineBindPoint pipeline_bind_point)
{
	// Create a new pipeline only if the graphics state changed
	if (!pipeline_state_.is_dirty())
	{
		return;
	}

	pipeline_state_.clear_dirty();

	// Create and bind pipeline
	if (pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
	{
		auto &pipeline = get_device().get_resource_cache().request_graphics_pipeline(pipeline_state_);

		get_handle().bindPipeline(pipeline_bind_point, pipeline.get_handle());
	}
	else if (pipeline_bind_point == vk::PipelineBindPoint::eCompute)
	{
		auto &pipeline = get_device().get_resource_cache().request_compute_pipeline(pipeline_state_);

		get_handle().bindPipeline(pipeline_bind_point, pipeline.get_handle());
	}
	else
	{
		throw R"(Only graphics and compute pipeline bind points are supported now)";
	}
}

void CommandBuffer::flush_push_constants()
{
	if (stored_push_constants_.empty())
	{
		return;
	}

	const backend::PipelineLayout &pipeline_layout = pipeline_state_.get_pipeline_layout();

	vk::ShaderStageFlags shader_stage = pipeline_layout.get_push_constant_range_stage(to_u32(stored_push_constants_.size()));

	if (shader_stage)
	{
		get_handle().pushConstants<uint8_t>(pipeline_layout.get_handle(), shader_stage, 0, stored_push_constants_);
	}
	else
	{
		LOGW("Push constant range [{}, {}] not found", 0, stored_push_constants_.size());
	}

	stored_push_constants_.clear();
}

}        // namespace xihe::backend
