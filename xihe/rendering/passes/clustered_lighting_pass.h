#pragma once

#include "lighting_pass.h"

#include "scene_graph/components/camera.h"
#include "scene_graph/components/light.h"
#include "scene_graph/scripts/cascade_script.h"

namespace xihe::rendering
{
constexpr uint32_t kMaxPointLightCount = 256;

struct SortedLight
{
	uint32_t light_index;
	uint32_t point_light_index;
	float    projected_z;
	float    projected_z_min;
	float    projected_z_max;
};

struct alignas(16) ClusteredLights
{
	Light directional_lights[kMaxLightCount];
	Light point_lights[kMaxPointLightCount];
	Light spot_lights[kMaxLightCount];
};

struct alignas(16) ClusteredLightUniform
{
	glm::mat4 inv_view_proj;
	glm::mat4 view;
	float     near_plane;
	float     far_plane;
};

class ClusteredLightingPass : public LightingPass
{
  public:
	ClusteredLightingPass(std::vector<sg::Light *> lights, sg::Camera &camera, sg::CascadeScript *cascade_script = nullptr);

	~ClusteredLightingPass() override = default;

	void generate_lighting_data();

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:

	void collect_and_sort_lights();

	void generate_bins();

	void generate_tiles();

	std::vector<SortedLight> sorted_lights_;

	std::vector<uint32_t> light_indices_;
	std::vector<uint32_t> bins_;         // 16 bit min, 16 bit max
	std::vector<uint32_t> tiles_;        // bitfield for each tile

	static constexpr uint32_t num_bins_   = 16;
	static constexpr uint32_t tile_size_  = 8;
	static constexpr uint32_t max_point_light_count_ = 256;
	static constexpr uint32_t num_words_  = (max_point_light_count_ + 31) / 32;

	uint32_t width_{};
	uint32_t height_{};
	uint32_t num_tiles_x_{};
	uint32_t num_tiles_y_{};
};
}        // namespace xihe::rendering
