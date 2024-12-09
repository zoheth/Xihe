#pragma once

#include "geometry_pass.h"
#include "scene_graph/scripts/cascade_script.h"

namespace xihe::rendering
{
constexpr uint32_t kShadowmapResolution = 2048;

class CascadeShadowPass : public RenderPass
{
  public:
	CascadeShadowPass(std::vector<sg::Mesh *> meshes, sg::CascadeScript &cascade_script, uint32_t cascade_index);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

  private:
	void update_uniforms(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, sg::Node &node, size_t thread_index);

	void draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, const std::vector<backend::ShaderResource> &vertex_input_resources);

	std::vector<sg::Mesh *> meshes_;
	sg::CascadeScript &cascade_script_;

	uint32_t cascade_index_{};
};
}        // namespace xihe::rendering
