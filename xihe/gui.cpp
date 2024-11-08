#include "gui.h"

#include "xihe_app.h"
#include "rendering/pipeline_state.h"

namespace xihe
{
namespace
{
void upload_draw_data(const ImDrawData *draw_data, uint8_t *vertex_data, uint8_t *index_data)
{
	auto  vtx_dst = reinterpret_cast<ImDrawVert *>(vertex_data);
	auto *idx_dst = reinterpret_cast<ImDrawIdx *>(index_data);

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
}
}        // namespace

const std::string Gui::default_font_ = "Roboto-Regular";
bool              Gui::visible_      = true;

Gui::Gui(XiheApp &app, Window &window, const float font_size, bool explicit_update) :
    app_{app},
    explicit_update_{explicit_update}
{
	ImGui::CreateContext();

	ImGuiStyle &style = ImGui::GetStyle();

	// Color scheme
	style.Colors[ImGuiCol_WindowBg]         = ImVec4(0.005f, 0.005f, 0.005f, 0.94f);
	style.Colors[ImGuiCol_TitleBg]          = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header]           = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_FrameBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_CheckMark]        = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab]       = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
	style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
	style.Colors[ImGuiCol_Button]           = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_ButtonActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

	// Borderless window
	style.WindowBorderSize = 0.0f;

	// Global scale
	style.ScaleAllSizes(dpi_factor_);

	// Dimensions
	ImGuiIO &io                = ImGui::GetIO();
	auto     extent            = app.get_render_context().get_surface_extent();
	io.DisplaySize.x           = static_cast<float>(extent.width);
	io.DisplaySize.y           = static_cast<float>(extent.height);
	io.FontGlobalScale         = 1.0f;
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.KeyMap[ImGuiKey_Space]      = static_cast<int>(KeyCode::Space);
	io.KeyMap[ImGuiKey_Enter]      = static_cast<int>(KeyCode::Enter);
	io.KeyMap[ImGuiKey_LeftArrow]  = static_cast<int>(KeyCode::Left);
	io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(KeyCode::Right);
	io.KeyMap[ImGuiKey_UpArrow]    = static_cast<int>(KeyCode::Up);
	io.KeyMap[ImGuiKey_DownArrow]  = static_cast<int>(KeyCode::Down);
	io.KeyMap[ImGuiKey_Tab]        = static_cast<int>(KeyCode::Tab);
	io.KeyMap[ImGuiKey_Escape]     = static_cast<int>(KeyCode::Backspace);

	// Default font
	fonts_.emplace_back(default_font_, font_size * dpi_factor_);

	// Debug window font
	fonts_.emplace_back("RobotoMono-Regular", (font_size / 2) * dpi_factor_);

	unsigned char *font_data;
	int            tex_width, tex_height;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
	size_t upload_size = tex_width * tex_height * 4 * sizeof(char);

	auto &device = app.get_render_context().get_device();

	vk::Extent3D font_extent{to_u32(tex_width), to_u32(tex_height), 1u};

	backend::ImageBuilder image_builder{font_extent};
	image_builder.with_format(vk::Format::eR8G8B8A8Unorm)
	    .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
	    .with_vma_flags(VMA_MEMORY_USAGE_GPU_ONLY);
	font_image_ = image_builder.build_unique(device);
	font_image_->set_debug_name("GUI font image");

	font_image_view_ = std::make_unique<backend::ImageView>(*font_image_, vk::ImageViewType::e2D);
	font_image_view_->set_debug_name("View on GUI font image");

	{
		backend::Buffer stage_buffer = backend::Buffer::create_staging_buffer(device, upload_size, font_data);

		auto &command_buffer = device.request_command_buffer();

		backend::FencePool fence_pool{device};

		command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		{
			common::ImageMemoryBarrier memory_barrier{};
			memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
			memory_barrier.new_layout      = vk::ImageLayout::eTransferDstOptimal;
			memory_barrier.src_access_mask = {};
			memory_barrier.dst_access_mask = vk::AccessFlagBits2::eTransferWrite;
			memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eHost;
			memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;

			command_buffer.image_memory_barrier(*font_image_view_, memory_barrier);
		}

		vk::BufferImageCopy region{};
		region.imageSubresource.layerCount = font_image_view_->get_subresource_range().layerCount;
		region.imageSubresource.aspectMask = font_image_view_->get_subresource_range().aspectMask;
		region.imageExtent                 = font_extent;

		command_buffer.copy_buffer_to_image(stage_buffer, *font_image_, {region});

		{
			common::ImageMemoryBarrier memory_barrier{};
			memory_barrier.old_layout      = vk::ImageLayout::eTransferDstOptimal;
			memory_barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
			memory_barrier.src_access_mask = vk::AccessFlagBits2::eTransferWrite;
			memory_barrier.dst_access_mask = vk::AccessFlagBits2::eShaderRead;
			memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;
			memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;

			command_buffer.image_memory_barrier(*font_image_view_, memory_barrier);
		}

		command_buffer.end();

		auto &queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

		queue.submit(command_buffer, device.request_fence());

		// Wait for the command buffer to finish its work before destroying the staging buffer
		device.get_fence_pool().wait();
		device.get_fence_pool().reset();
		device.get_command_pool().reset_pool();
	}

	vk::Filter filter = vk::Filter::eLinear;
	common::make_filters_valid(device.get_gpu().get_handle(), font_image_->get_format(), &filter);

	vk::SamplerCreateInfo sampler_info{};
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.magFilter     = filter;
	sampler_info.minFilter     = filter;
	sampler_info.mipmapMode    = vk::SamplerMipmapMode::eNearest;
	sampler_info.addressModeU  = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV  = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW  = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.borderColor   = vk::BorderColor::eFloatOpaqueWhite;

	backend::ShaderSource vert_shader{"imgui.vert"};
	backend::ShaderSource frag_shader{"imgui.frag"};

	std::vector<backend::ShaderModule *> shader_modules;
	shader_modules.push_back(&device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, vert_shader, {}));
	shader_modules.push_back(&device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, frag_shader, {}));

	pipeline_layout_ = &device.get_resource_cache().request_pipeline_layout(shader_modules);

	sampler_ = std::make_unique<backend::Sampler>(device, sampler_info);
	sampler_->set_debug_name("GUI sampler");

	if (explicit_update_)
	{
		backend::BufferBuilder buffer_builder{1};
		buffer_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_TO_CPU);

		buffer_builder.with_usage(vk::BufferUsageFlagBits::eVertexBuffer);
		vertex_buffer_ = buffer_builder.build_unique(device);
		vertex_buffer_->set_debug_name("GUI vertex buffer");

		buffer_builder.with_usage(vk::BufferUsageFlagBits::eIndexBuffer);
		index_buffer_ = buffer_builder.build_unique(device);
		index_buffer_->set_debug_name("GUI index buffer");
	}
}

Gui::~Gui()
{
	app_.get_render_context().get_device().get_handle().destroyDescriptorPool(descriptor_pool_);
	app_.get_render_context().get_device().get_handle().destroyDescriptorSetLayout(descriptor_set_layout_);
	app_.get_render_context().get_device().get_handle().destroyPipeline(pipeline_);

	ImGui::DestroyContext();
}

void Gui::prepare(vk::PipelineCache pipeline_cache, vk::RenderPass render_pass, const std::vector<vk::PipelineShaderStageCreateInfo> &shader_stages)
{
	backend::Device &device = app_.get_render_context().get_device();

	std::vector<vk::DescriptorPoolSize> pool_sizes = {
	    {vk::DescriptorType::eCombinedImageSampler, 1},
	};
	vk::DescriptorPoolCreateInfo pool_info{{}, 2, pool_sizes};
	vk::Result                   result = device.get_handle().createDescriptorPool(&pool_info, nullptr, &descriptor_pool_);
	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Cannot create descriptor pool"};
	}

	std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
	    {0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	};
	vk::DescriptorSetLayoutCreateInfo layout_info{{}, layout_bindings};
	result = device.get_handle().createDescriptorSetLayout(&layout_info, nullptr, &descriptor_set_layout_);
	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Cannot create descriptor set layout"};
	}

	vk::DescriptorSetAllocateInfo alloc_info{descriptor_pool_, 1, &descriptor_set_layout_};
	result = device.get_handle().allocateDescriptorSets(&alloc_info, &descriptor_set_);
	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Cannot allocate descriptor set"};
	}
	vk::DescriptorImageInfo             font_image_info{sampler_->get_handle(), font_image_view_->get_handle(), vk::ImageLayout::eShaderReadOnlyOptimal};
	std::vector<vk::WriteDescriptorSet> write_descriptor_sets = {
	    {descriptor_set_, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &font_image_info},
	};
	device.get_handle().updateDescriptorSets(write_descriptor_sets, {});

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{{}, vk::PrimitiveTopology::eTriangleList, VK_FALSE};

	vk::PipelineRasterizationStateCreateInfo rasterization_state{};
	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	rasterization_state.cullMode    = vk::CullModeFlagBits::eNone;
	rasterization_state.frontFace   = vk::FrontFace::eCounterClockwise;

	vk::PipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable         = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	color_blend_attachment.colorBlendOp        = vk::BlendOp::eAdd;
	color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	color_blend_attachment.alphaBlendOp        = vk::BlendOp::eAdd;
	color_blend_attachment.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo color_blend_state{};
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = &color_blend_attachment;

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{};
	depth_stencil_state.depthTestEnable  = VK_FALSE;
	depth_stencil_state.depthWriteEnable = VK_FALSE;

	vk::PipelineViewportStateCreateInfo viewport_state{{}, 1, nullptr, 1, nullptr};

	vk::PipelineMultisampleStateCreateInfo multisample_state{};
	multisample_state.rasterizationSamples = vk::SampleCountFlagBits::e1;

	std::vector<vk::DynamicState> dynamic_states = {
	    vk::DynamicState::eViewport,
	    vk::DynamicState::eScissor,
	};
	vk::PipelineDynamicStateCreateInfo dynamic_state{{}, dynamic_states};

	vk::GraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.stageCount          = to_u32(shader_stages.size());
	pipeline_info.pStages             = shader_stages.data();
	pipeline_info.pInputAssemblyState = &input_assembly_state;
	pipeline_info.pRasterizationState = &rasterization_state;
	pipeline_info.pColorBlendState    = &color_blend_state;
	pipeline_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_info.pViewportState      = &viewport_state;
	pipeline_info.pMultisampleState   = &multisample_state;
	pipeline_info.pDynamicState       = &dynamic_state;

	std::vector<vk::VertexInputBindingDescription> vertex_input_bindings = {
	    {0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex},
	};
	std::vector<vk::VertexInputAttributeDescription> vertex_input_attributes = {
	    {0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)},
	    {1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)},
	    {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)},
	};
	vk::PipelineVertexInputStateCreateInfo vertex_input_state{{}, vertex_input_bindings, vertex_input_attributes};

	pipeline_info.pVertexInputState = &vertex_input_state;

	/*result = device.get_handle().createGraphicsPipelines(pipeline_cache, 1, &pipeline_info, nullptr, &pipeline_);
	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Cannot create graphics pipeline"};
	}*/
}

void Gui::update(const float delta_time)
{
	if (visible_ != prev_visible_)
	{
		prev_visible_ = visible_;
	}

	if (!visible_)
	{
		ImGui::EndFrame();
		return;
	}

	ImGuiIO &io      = ImGui::GetIO();
	auto     extent  = app_.get_render_context().get_surface_extent();
	io.DisplaySize.x = static_cast<float>(extent.width);
	io.DisplaySize.y = static_cast<float>(extent.height);
	io.DeltaTime     = delta_time;

	// Render to generate draw buffers
	ImGui::Render();
}

void Gui::draw(backend::CommandBuffer &command_buffer)
{
	if (!visible_)
	{
		return;
	}

	backend::ScopedDebugLabel debug_label{command_buffer, "GUI"};

	vk::VertexInputBindingDescription vertex_input_binding{0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex};

	std::vector<vk::VertexInputAttributeDescription> vertex_input_attributes = {
	    {0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)},
	    {1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)},
	    {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)},
	};

	VertexInputState vertex_input_state{{vertex_input_binding}, vertex_input_attributes};

	command_buffer.set_vertex_input_state(vertex_input_state);

	ColorBlendAttachmentState color_attachment{};
	color_attachment.blend_enable           = true;
	color_attachment.color_write_mask       = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
	color_attachment.src_color_blend_factor = vk::BlendFactor::eSrcAlpha;
	color_attachment.dst_color_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;
	color_attachment.src_alpha_blend_factor = vk::BlendFactor::eOneMinusSrcAlpha;

	ColorBlendState color_blend_state{};
	color_blend_state.attachments.push_back(color_attachment);

	command_buffer.set_color_blend_state(color_blend_state);

	RasterizationState rasterization_state{};
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	DepthStencilState depth_stencil_state{};
	depth_stencil_state.depth_test_enable = VK_FALSE;
	depth_stencil_state.depth_write_enable = VK_FALSE;
	command_buffer.set_depth_stencil_state(depth_stencil_state);

	command_buffer.bind_pipeline_layout(*pipeline_layout_);
	command_buffer.bind_image(*font_image_view_, *sampler_, 0, 0, 0);

	command_buffer.push_constants(glm::mat4(1.0f));

	if (!explicit_update_)
	{
		update_buffers(command_buffer);
	}
	else
	{
		std::vector<std::reference_wrapper<const backend::Buffer>> buffers;
		buffers.emplace_back(*vertex_buffer_);
		command_buffer.bind_vertex_buffers(0, buffers, {0});

		command_buffer.bind_index_buffer(*index_buffer_, 0, vk::IndexType::eUint16);
	}

	ImDrawData *draw_data     = ImGui::GetDrawData();
	int32_t     vertex_offset = 0;
	uint32_t    index_offset  = 0;

	if (!draw_data || draw_data->CmdListsCount == 0)
	{
		return;
	}

	for (int32_t i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[i];
		for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; ++j)
		{
			const ImDrawCmd *cmd = &cmd_list->CmdBuffer[j];
			if (cmd->UserCallback)
			{
				cmd->UserCallback(cmd_list, cmd);
			}
			else
			{
				vk::Rect2D scissor_rect{
				    {static_cast<int32_t>(cmd->ClipRect.x), static_cast<int32_t>(cmd->ClipRect.y)},
				    {static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x), static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y)},
				};
				command_buffer.set_scissor(0, {scissor_rect});
				command_buffer.draw_indexed(cmd->ElemCount, 1, index_offset, vertex_offset, 0);
			}
			index_offset += cmd->ElemCount;
		}
		vertex_offset += cmd_list->VtxBuffer.Size;
	}
}

bool Gui::input_event(const InputEvent &input_event)
{
	return true;
}

void Gui::show_simple_window(const std::string &name, uint32_t last_fps, const std::function<void()> &body) const
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::NewFrame();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(name.c_str());
	//ImGui::TextUnformatted(std::string(app_.get_render_context().get_device().get_gpu().get_properties().deviceName).c_str());
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / last_fps), last_fps);
	ImGui::PushItemWidth(110.0f * dpi_factor_);

	body();

	ImGui::PopItemWidth();
	ImGui::End();
	ImGui::PopStyleVar();
}

void Gui::new_frame()
{
	//ImGui::NewFrame();
}

void Gui::update_buffers(backend::CommandBuffer &command_buffer) const
{
	ImDrawData             *draw_data    = ImGui::GetDrawData();
	rendering::RenderFrame &render_frame = app_.get_render_context().get_active_frame();

	if (!draw_data || draw_data->TotalIdxCount == 0 || draw_data->TotalVtxCount)
	{
		return;
	}

	size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	size_t index_buffer_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	std::vector<uint8_t> vertex_data(vertex_buffer_size);
	std::vector<uint8_t> index_data(index_buffer_size);

	upload_draw_data(draw_data, vertex_data.data(), index_data.data());

	auto vertex_allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eVertexBuffer, vertex_buffer_size, 0);
	vertex_allocation.update(vertex_data);

	std::vector<std::reference_wrapper<const backend::Buffer>> vertex_buffers;
	vertex_buffers.emplace_back(std::ref(vertex_allocation.get_buffer()));

	auto index_allocation = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eIndexBuffer, index_buffer_size, 0);
	index_allocation.update(index_data);

	command_buffer.bind_vertex_buffers(0, vertex_buffers, {vertex_allocation.get_offset()});
	command_buffer.bind_index_buffer(index_allocation.get_buffer(), index_allocation.get_offset(), vk::IndexType::eUint16);
}
}        // namespace xihe
