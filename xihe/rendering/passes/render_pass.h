#pragma once
#include "backend/command_buffer.h"
#include "backend/shader_module.h"
#include "rendering/render_frame.h"

#include <optional>

namespace xihe
{
namespace rendering
{
glm::mat4 vulkan_style_projection(const glm::mat4 &proj);

class RenderPass
{
public:
	RenderPass() = default;

	void set_shader(std::initializer_list<std::string> file_names);

	const backend::ShaderSource &get_vertex_shader() const;
	const backend::ShaderSource &get_task_shader() const;
	const backend::ShaderSource &get_mesh_shader() const;
	const backend::ShaderSource &get_fragment_shader() const;
	const backend::ShaderSource &get_compute_shader() const;

	virtual void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame) {};

private:
	std::optional<backend::ShaderSource> vertex_shader_;

	std::optional<backend::ShaderSource> task_shader_;
	std::optional<backend::ShaderSource> mesh_shader_;

	std::optional<backend::ShaderSource> fragment_shader_;

	std::optional<backend::ShaderSource> compute_shader_;
};
}
}
