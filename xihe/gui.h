#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <imgui.h>
#include <imgui_internal.h>
#include <thread>

#include "backend/buffer.h"
#include "backend/command_buffer.h"
#include "backend/sampler.h"
#include "common/timer.h"
#include "platform/filesystem.h"
#include "platform/input_events.h"
#include "rendering/render_context.h"
#include "stats/stats.h"

namespace xihe
{
class XiheApp;
class Window;

struct Font
{
	/**
	 * @brief Constructor
	 * @param name The name of the font file that exists within 'assets/fonts' (without extension)
	 * @param size The font size, scaled by DPI
	 */
	Font(const std::string &name, float size) :
	    name{name},
	    data{fs::read_asset("fonts/" + name + ".ttf")},
	    size{size}
	{
		// Keep ownership of the font data to avoid a double delete
		ImFontConfig font_config{};
		font_config.FontDataOwnedByAtlas = false;

		if (size < 1.0f)
		{
			size = 20.0f;
		}

		ImGuiIO &io = ImGui::GetIO();
		handle      = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config, io.Fonts->GetGlyphRangesChineseFull());
	}

	ImFont *handle{nullptr};

	std::string name;

	std::vector<uint8_t> data;

	float size{};
};

class Gui
{
  public:
	struct StatsView
	{
		StatsView(const stats::Stats *stats);

		void reset_max_values();

		void reset_max_values(const stats::StatIndex index);

		std::map<stats::StatIndex, stats::StatGraphData> graph_map;

		float graph_height{50.0f};

		float top_padding{1.1f};
	};

	Gui(XiheApp &app, Window &window, const stats::Stats *stats = nullptr, const float font_size = 21.0f, bool explicit_update = false);

	~Gui();

	void prepare(vk::PipelineCache pipeline_cache);

	void update(const float delta_time);

	void draw(backend::CommandBuffer &command_buffer);

	bool input_event(const InputEvent &input_event);

	void show_simple_window(const std::string &name, uint32_t last_fps, const std::function<void()> &body);

	void show_views_window(std::function<void()> body, const uint32_t lines) const;

	void show_stats(const stats::Stats &stats);

	static void new_frame();

	Font &get_font(const std::string &font_name = Gui::default_font_);

  public:
	static const std::string default_font_;
	static bool              visible_;

  private:
	void update_buffers(backend::CommandBuffer &command_buffer) const;

	static constexpr uint32_t buffer_pool_block_size_ = 256;

	XiheApp &app_;

	std::unique_ptr<backend::Sampler> sampler_{nullptr};

	std::unique_ptr<backend::Buffer> vertex_buffer_{nullptr};
	std::unique_ptr<backend::Buffer> index_buffer_{nullptr};

	size_t last_vertex_buffer_size_{0};
	size_t last_index_buffer_size_{0};

	///  Scale factor to apply due to a difference between the window and GL pixel sizes
	float content_scale_factor_{1.0f};

	/// Scale factor to apply to the size of gui elements (expressed in dp)
	float dpi_factor_{1.0f};

	bool explicit_update_{false};

	std::vector<Font> fonts_;

	std::unique_ptr<backend::Image>     font_image_;
	std::unique_ptr<backend::ImageView> font_image_view_;

	backend::PipelineLayout *pipeline_layout_{nullptr};

	vk::DescriptorPool      descriptor_pool_{VK_NULL_HANDLE};
	vk::DescriptorSetLayout descriptor_set_layout_{VK_NULL_HANDLE};
	vk::DescriptorSet       descriptor_set_{VK_NULL_HANDLE};
	vk::Pipeline            pipeline_{VK_NULL_HANDLE};

	StatsView stats_view_;

	Timer timer_;

	bool prev_visible_{true};
};

}        // namespace xihe