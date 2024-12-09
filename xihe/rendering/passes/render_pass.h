#pragma once

#include "backend/command_buffer.h"
#include "backend/shader_module.h"
#include "rendering/render_frame.h"
#include "rendering/render_graph/render_resource.h"
#include "shared_uniform.h"

#include <optional>
#include <variant>

namespace xihe
{
namespace rendering
{

glm::mat4 vulkan_style_projection(const glm::mat4 &proj);

class RenderPass
{
  public:
	RenderPass() = default;

	PassType get_type() const;

	void set_shader(std::initializer_list<std::string> file_names);

	const backend::ShaderSource &get_vertex_shader() const;
	const backend::ShaderSource &get_task_shader() const;
	const backend::ShaderSource &get_mesh_shader() const;
	const backend::ShaderSource &get_fragment_shader() const;
	const backend::ShaderSource &get_compute_shader() const;

	/**
	 * \brief
	 * \param command_buffer
	 * \param active_frame  Used to obtain resources for the current frame, or allocate resources
	 * \param input_bindables The order of input_bindables corresponds to the order of inputs passed during add_pass
	 */
	virtual void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables);

  protected:
	uint32_t thread_index_{0};

  private:
	PassType type_{};

	std::optional<backend::ShaderSource> vertex_shader_;

	std::optional<backend::ShaderSource> task_shader_;
	std::optional<backend::ShaderSource> mesh_shader_;

	std::optional<backend::ShaderSource> fragment_shader_;

	std::optional<backend::ShaderSource> compute_shader_;
};
}        // namespace rendering
}        // namespace xihe
