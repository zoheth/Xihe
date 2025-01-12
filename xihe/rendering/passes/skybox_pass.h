#pragma once

#include "render_pass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/texture.h"

namespace xihe::rendering
{
class SkyboxPass : public RenderPass
{
  public:
	SkyboxPass(sg::Mesh &box_mesh, rendering::Texture &cubemap, sg::Camera &camera);
	SkyboxPass(sg::Mesh &box_mesh, sg::Texture &cubemap, sg::Camera &camera);

	~SkyboxPass() override = default;

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:
	sg::Mesh    &box_mesh_;
	Texture     *cubemap_{nullptr};
	sg::Texture *cubemap_sg_{nullptr};
	sg::Camera  &camera_;
};
}        // namespace xihe::rendering
