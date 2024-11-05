#pragma once

#include "rendering/subpass.h"

#include "scene_graph/components/mshader_mesh.h"

namespace xihe
{

namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg

namespace rendering
{
struct UBOVS
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class MeshletSubpass : public rendering::Subpass
{
  public:
	MeshletSubpass(rendering::RenderContext &render_context, std::optional<backend::ShaderSource> task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera);

	void prepare() override;

	void draw(backend::CommandBuffer &command_buffer) override;

	void update_uniform(backend::CommandBuffer &command_buffer, size_t thread_index) const;

  private:
	sg::Camera &camera_;

	sg::Scene &scene_;

	sg::Mesh *mesh_{nullptr};
	sg::MshaderMesh *mshader_mesh_{nullptr};
};

}        // namespace rendering
}        // namespace xihe
