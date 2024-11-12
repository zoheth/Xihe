#include "gui.h"

#include "rendering/pipeline_state.h"
#include "xihe_app.h"
#include "fmt/format.h"

#include <numeric>

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

void reset_graph_max_value(stats::StatGraphData &graph_data)
{
	// If it does not have a fixed max
	if (!graph_data.has_fixed_max)
	{
		// Reset it
		graph_data.max_value = 0.0f;
	}
}
}        // namespace

const std::string Gui::default_font_ = "msyh";
bool              Gui::visible_      = true;

Gui::Gui(XiheApp &app, Window &window, const stats::Stats *stats, const float font_size, bool explicit_update) :
    app_{app},
    explicit_update_{explicit_update},
    stats_view_{stats}
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

Gui::StatsView::StatsView(const stats::Stats *stats)
{
	if (stats == nullptr)
	{
		return;
	}

	const std::set<stats::StatIndex> &requested_stats = stats->get_requested_stats();

	for (stats::StatIndex index : requested_stats)
	{
		graph_map[index] = stats->get_graph_data(index);
	}
}

void Gui::StatsView::reset_max_values()
{
	// For every entry in the map
	std::ranges::for_each(graph_map,
	                      [](auto &pr) { reset_graph_max_value(pr.second); });
}

void Gui::StatsView::reset_max_values(const stats::StatIndex index)
{
	auto it = graph_map.find(index);
	if (it != graph_map.end())
	{
		reset_graph_max_value(it->second);
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
	depth_stencil_state.depth_test_enable  = VK_FALSE;
	depth_stencil_state.depth_write_enable = VK_FALSE;
	command_buffer.set_depth_stencil_state(depth_stencil_state);

	command_buffer.bind_pipeline_layout(*pipeline_layout_);
	command_buffer.bind_image(*font_image_view_, *sampler_, 0, 0, 0);

	auto &io             = ImGui::GetIO();
	auto  push_transform = glm::mat4(1.0f);
	// GUI coordinate space to screen space
	push_transform = glm::translate(push_transform, glm::vec3(-1.0f, -1.0f, 0.0f));
	push_transform = glm::scale(push_transform, glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));

	command_buffer.push_constants(push_transform);

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
	auto &io                 = ImGui::GetIO();
	auto  capture_move_event = false;

	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_action() == KeyAction::Down)
		{
			io.KeysDown[static_cast<int>(key_event.get_code())] = true;
		}
		else if (key_event.get_action() == KeyAction::Up)
		{
			io.KeysDown[static_cast<int>(key_event.get_code())] = false;
		}
	}
	else if (input_event.get_source() == EventSource::Mouse)
	{
		const auto &mouse_button = static_cast<const MouseButtonInputEvent &>(input_event);

		io.MousePos = ImVec2{mouse_button.get_pos_x() * content_scale_factor_,
		                     mouse_button.get_pos_y() * content_scale_factor_};

		auto button_id = static_cast<int>(mouse_button.get_button());

		if (mouse_button.get_action() == MouseAction::Down)
		{
			io.MouseDown[button_id] = true;
		}
		else if (mouse_button.get_action() == MouseAction::Up)
		{
			io.MouseDown[button_id] = false;
		}
		else if (mouse_button.get_action() == MouseAction::Move)
		{
			capture_move_event = io.WantCaptureMouse;
		}
	}
	else if (input_event.get_source() == EventSource::Touchscreen)
	{
		const auto &touch_event = static_cast<const TouchInputEvent &>(input_event);

		io.MousePos = ImVec2{touch_event.get_pos_x(), touch_event.get_pos_y()};

		if (touch_event.get_action() == TouchAction::Down)
		{
			io.MouseDown[touch_event.get_pointer_id()] = true;
		}
		else if (touch_event.get_action() == TouchAction::Up)
		{
			io.MouseDown[touch_event.get_pointer_id()] = false;
		}
		else if (touch_event.get_action() == TouchAction::Move)
		{
			capture_move_event = io.WantCaptureMouse;
		}
	}

	// Toggle GUI elements when tap or clicking outside the GUI windows
	if (!io.WantCaptureMouse)
	{
		bool press_down = (input_event.get_source() == EventSource::Mouse && static_cast<const MouseButtonInputEvent &>(input_event).get_action() == MouseAction::Down) || (input_event.get_source() == EventSource::Touchscreen && static_cast<const TouchInputEvent &>(input_event).get_action() == TouchAction::Down);
		bool press_up   = (input_event.get_source() == EventSource::Mouse && static_cast<const MouseButtonInputEvent &>(input_event).get_action() == MouseAction::Up) || (input_event.get_source() == EventSource::Touchscreen && static_cast<const TouchInputEvent &>(input_event).get_action() == TouchAction::Up);

		if (press_down)
		{
			timer_.start();
			if (input_event.get_source() == EventSource::Touchscreen)
			{
				const auto &touch_event = static_cast<const TouchInputEvent &>(input_event);
				/*if (touch_event.get_touch_points() == 2)
				{
				    two_finger_tap_ = true;
				}*/
			}
		}
	}

	return capture_move_event;
}

void Gui::show_simple_window(const std::string &name, uint32_t last_fps, const std::function<void()> &body)
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("测试窗口", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImGui::TextUnformatted(name.c_str());
	// ImGui::TextUnformatted(std::string(app_.get_render_context().get_device().get_gpu().get_properties().deviceName).c_str());
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / last_fps), last_fps);
	ImGui::PushItemWidth(110.0f * dpi_factor_);

	body();

	ImGui::PopItemWidth();

	ImGui::End();
	ImGui::PopStyleVar();
}

void Gui::show_views_window(std::function<void()> body, const uint32_t lines) const
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("切换视图", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImGui::PushItemWidth(110.0f * dpi_factor_);

	body();

	ImGui::PopItemWidth();

	ImGui::End();
	ImGui::PopStyleVar();
}

void Gui::show_stats(const stats::Stats &stats)
{
	ImGuiIO &io = ImGui::GetIO();
	ImVec2 pos = ImVec2(io.DisplaySize.x - 200, 10);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

	ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

	for (const auto &stat_index : stats.get_requested_stats())
	{
		auto it = stats_view_.graph_map.find(stat_index);

		assert(it != stats_view_.graph_map.end() && "StatIndex not implemented in gui graph_map");

		auto       &graph_data     = it->second;
		const auto &graph_elements = stats.get_data(stat_index);
		float       graph_min      = 0.0f;
		float      &graph_max      = graph_data.max_value;

		if (!graph_data.has_fixed_max)
		{
			auto new_max = *std::max_element(graph_elements.begin(), graph_elements.end()) * stats_view_.top_padding;
			if (new_max > graph_max)
			{
				graph_max = new_max;
			}
		}

		ImVec2 graph_size = ImVec2(io.DisplaySize.x - pos.x - 10, stats_view_.graph_height * dpi_factor_);



		std::stringstream graph_label;
		float             avg = std::accumulate(graph_elements.begin(), graph_elements.end(), 0.0f) / graph_elements.size();

		if (stats.is_available(stat_index))
		{
			graph_label << graph_data.name << ": " << graph_data.format;
			std::string text = fmt::format(fmt::runtime(graph_label.str()), avg * graph_data.scale_factor);
			ImGui::PlotLines(text.c_str(), graph_elements.data(), static_cast<int>(graph_elements.size()), 0, nullptr, graph_min, graph_max, graph_size);

			ImGui::Text(text.c_str());
			ImGui::Separator(); 
		}
		else
		{
			graph_label << graph_data.name << ": not available";
			ImGui::Text("%s", graph_label.str().c_str());
		}
	}
	ImGui::End();
}

void Gui::new_frame()
{
	ImGui::NewFrame();
}

Font &Gui::get_font(const std::string &font_name)
{
	assert(!fonts_.empty() && "No fonts exist");

	const auto it = std::ranges::find_if(fonts_, [&font_name](const Font &font) { return font.name == font_name; });

	if (it != fonts_.end())
	{
		return *it;
	}
	else
	{
		LOGW("Couldn't find font with name {}", font_name);
		return *fonts_.begin();
	}
}

void Gui::update_buffers(backend::CommandBuffer &command_buffer) const
{
	ImDrawData             *draw_data    = ImGui::GetDrawData();
	rendering::RenderFrame &render_frame = app_.get_render_context().get_active_frame();

	if (!draw_data || draw_data->TotalIdxCount == 0 || draw_data->TotalVtxCount == 0)
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
