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
struct alignas(16) MeshletSceneUniform
{
	glm::mat4 model;

	glm::mat4 view;

	glm::mat4 camera_view_proj;

	glm::vec4 frustum_planes[6];

	glm::vec3 camera_position;
};

class MeshletSubpass : public rendering::Subpass
{
  public:
	MeshletSubpass(rendering::RenderContext &render_context, std::optional<backend::ShaderSource> task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera);

	void prepare() override;

	void draw(backend::CommandBuffer &command_buffer) override;

	void update_uniform(backend::CommandBuffer &command_buffer, sg::Node &node, size_t thread_index) const;

	static void show_meshlet_view(bool show, sg::Scene &scene);

	static void freeze_frustum(bool freeze, sg::Camera *camera=nullptr);

  private:

	backend::PipelineLayout &prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules);

	void draw_mshader_mesh(backend::CommandBuffer &command_buffer, sg::MshaderMesh &mshader_mesh);

	  void get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &opaque_nodes,
	                      std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &transparent_nodes) const;

	sg::Camera &camera_;

	sg::Scene &scene_;

	std::vector<sg::Mesh *> meshes_;

	inline static bool show_debug_view_{false};

	inline static bool freeze_frustum_{false};
	inline static glm::mat4 frozen_view_;
	inline static glm::vec4 frozen_frustum_planes_[6];

	// sg::MshaderMesh *mshader_mesh_{nullptr};
};

}        // namespace rendering
}        // namespace xihe
