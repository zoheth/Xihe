#pragma once
#include "scene_graph/asset_loader.h"
#include "scene_graph/components/mesh.h"
#include "xihe_app.h"

#define M_PI 3.14159265358979323846        // pi

namespace xihe
{

struct PushBlockIrradiance
{
	glm::mat4 mvp;
	float     deltaPhi   = (2.0f * static_cast<float>(M_PI)) / 180.0f;
	float     deltaTheta = (0.5f * static_cast<float>(M_PI)) / 64.0f;
};

struct PushBlockPrefilterEnv
{
	glm::mat4 mvp;
	float     roughness;
	uint32_t  num_samples = 32u;
};

class PrefilterPass : public rendering::RenderPass
{
  public:
	PrefilterPass(sg::Mesh &sky_box, sg::Texture &cubemap, uint32_t mip, uint32_t face);

	~PrefilterPass() override = default;

	void execute(backend::CommandBuffer &command_buffer, rendering::RenderFrame &active_frame, std::vector<rendering::ShaderBindable> input_bindables) override;

	inline static uint32_t num_mips = 0;
  private:
	sg::Texture &cubemap_;
	sg::Mesh &sky_box_;

	uint32_t mip_{};
	uint32_t face_{};
};

class PreprocessApp : public XiheApp
{
  public:
	PreprocessApp();

	~PreprocessApp() override = default;

	bool prepare(Window *window) override;

	void update(float delta_time) override;

	void request_gpu_features(backend::PhysicalDevice &gpu) override;

  private:
	void draw_gui() override;

	std::unique_ptr<AssetLoader> asset_loader_;
};
}        // namespace xihe
