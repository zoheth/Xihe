#pragma once
#include "rendering/passes/preprocess.h"
#include "scene_graph/asset_loader.h"
#include "scene_graph/components/mesh.h"
#include "xihe_app.h"

namespace xihe
{
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

	std::unique_ptr<sg::Mesh> sky_box_;

	struct Textures
	{
		sg::Texture       *environment_cube;
		rendering::Texture empty;
		rendering::Texture lut_brdf;
		rendering::Texture irradiance_cube;
		rendering::Texture prefiltered_cube;
	} textures_;
};
}        // namespace xihe
