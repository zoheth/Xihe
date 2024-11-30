#pragma once
#include "render_pass.h"
#include "backend/command_buffer.h"
#include "backend/resources_management/resource_cache.h"
#include "backend/shader_module.h"
#include "rendering/render_frame.h"

#include <optional>

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
class GeometryPass : public RenderPass
{
  public:
	GeometryPass(std::vector<sg::Mesh *> meshes, sg::Camera &camera);

	virtual ~GeometryPass() = default;

	/**
	 * \brief
	 * \param command_buffer
	 * \param active_frame  Used to obtain resources for the current frame, or allocate resources
	 */
	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame) override;

  private:
	virtual void update_uniform(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, sg::Node &node, size_t thread_index);

	void draw_submesh(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh, vk::FrontFace front_face = vk::FrontFace::eCounterClockwise);

	virtual void prepare_pipeline_state(backend::CommandBuffer &command_buffer, vk::FrontFace front_face, bool double_sided_material);

	virtual void prepare_push_constants(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

	virtual void draw_submesh_command(backend::CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

	/**
	 * \brief Sorts objects based on distance from camera and classifies them
	 *        into opaque and transparent in the arrays provided
	 */
	void get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &opaque_nodes,
	                      std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &transparent_nodes) const;

	std::vector<sg::Mesh *> meshes_;
	sg::Camera             &camera_;

	uint32_t color_attachments_count_{2};
};
}        // namespace rendering
}        // namespace xihe
