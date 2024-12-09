#pragma once

#include "render_pass.h"

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

class MeshletPass : public RenderPass
{
  public:
	MeshletPass(std::vector<sg::Mesh *> meshes, sg::Camera &camera);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

	static void show_meshlet_view(bool show, sg::Scene &scene);

	static void freeze_frustum(bool freeze, sg::Camera *camera);

  private:
	void update_uniform(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, sg::Node &node, size_t thread_index);

	void draw_mshader_mesh(backend::CommandBuffer &command_buffer, sg::MshaderMesh &mshader_mesh);

	void get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &opaque_nodes,
	                      std::multimap<float, std::pair<sg::Node *, sg::MshaderMesh *>> &transparent_nodes) const;

	std::vector<sg::Mesh *> meshes_;
	sg::Camera             &camera_;

	inline static bool show_debug_view_{false};

	inline static bool      freeze_frustum_{false};
	inline static glm::mat4 frozen_view_;
	inline static glm::vec4 frozen_frustum_planes_[6];
};
}        // namespace rendering
}        // namespace xihe