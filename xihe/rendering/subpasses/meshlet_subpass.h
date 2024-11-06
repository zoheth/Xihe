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

	void update_uniform(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index) const;

  private:

	backend::PipelineLayout &prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules);

	void draw_mshader_mesh(backend::CommandBuffer &command_buffer, sg::MshaderMesh &mshader_mesh);

	  void get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &opaque_nodes,
	                      std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &transparent_nodes) const;

	sg::Camera &camera_;

	sg::Scene &scene_;

	std::vector<sg::Mesh *> meshes_;

	// sg::MshaderMesh *mshader_mesh_{nullptr};
};

}        // namespace rendering
}        // namespace xihe
