#include "render_pass.h"

namespace xihe::rendering
{
glm::mat4 vulkan_style_projection(const glm::mat4 &proj)
{
	// Flip Y in clipspace. X = -1, Y = -1 is topLeft in Vulkan.
	glm::mat4 mat = proj;
	mat[1][1] *= -1;

	return mat;
}

void RenderPass::set_shader(std::initializer_list<std::string> file_names)
{
	for (const auto &file_name : file_names)
	{
		size_t dot_pos = file_name.find_last_of('.');
		if (dot_pos == std::string::npos)
		{
			continue;
		}

		std::string extension = file_name.substr(dot_pos + 1);

		if (extension == "vert")
		{
			vertex_shader_ = backend::ShaderSource{file_name};
		}
		else if (extension == "frag")
		{
			fragment_shader_ = backend::ShaderSource{file_name};
		}
		else if (extension == "comp")
		{
			compute_shader_ = backend::ShaderSource{file_name};
		}
		else if (extension == "task")
		{
			task_shader_ = backend::ShaderSource{file_name};
		}
		else if (extension == "mesh")
		{
			mesh_shader_ = backend::ShaderSource{file_name};
		}
	}
}

const backend::ShaderSource & RenderPass::get_vertex_shader() const
{
	if (!vertex_shader_.has_value())
	{
		throw std::runtime_error("Vertex shader not set");
	}
	return vertex_shader_.value();
}

const backend::ShaderSource & RenderPass::get_task_shader() const
{
	if (!task_shader_.has_value())
	{
		throw std::runtime_error("Task shader not set");
	}
	return task_shader_.value();
}

const backend::ShaderSource & RenderPass::get_mesh_shader() const
{
	if (!mesh_shader_.has_value())
	{
		throw std::runtime_error("Mesh shader not set");
	}
	return mesh_shader_.value();
}

const backend::ShaderSource & RenderPass::get_fragment_shader() const
{
	if (!fragment_shader_.has_value())
	{
		throw std::runtime_error("Fragment shader not set");
	}
	return fragment_shader_.value();
}

const backend::ShaderSource & RenderPass::get_compute_shader() const
{
	if (!compute_shader_.has_value())
	{
		throw std::runtime_error("Compute shader not set");
	}
	return compute_shader_.value();
}

void RenderPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	throw std::runtime_error("RenderPass::execute not implemented");
}
}
