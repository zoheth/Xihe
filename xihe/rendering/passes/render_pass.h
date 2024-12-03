#pragma once

#include "backend/command_buffer.h"
#include "backend/shader_module.h"
#include "rendering/render_frame.h"
#include "shared_uniform.h"

#include <optional>
#include <variant>

namespace xihe
{
namespace rendering
{

glm::mat4 vulkan_style_projection(const glm::mat4 &proj);

class ShaderBindable
{
  public:
	ShaderBindable() = default;

	ShaderBindable(backend::Buffer *buffer) :
	    resource_{buffer}
	{}
	ShaderBindable(backend::ImageView *image_view) :
	    resource_{image_view}
	{}

	ShaderBindable &operator=(std::variant<backend::Buffer *, backend::ImageView *> resource)
	{
		resource_ = resource;
		return *this;
	}

	backend::ImageView &image_view() const
	{
		if (std::holds_alternative<backend::ImageView *>(resource_))
		{
			return *std::get<backend::ImageView *>(resource_);
		}
		throw std::runtime_error("Resource is not an image view");
	}

	backend::Buffer &buffer() const
	{
		if (std::holds_alternative<backend::Buffer *>(resource_))
		{
			return *std::get<backend::Buffer *>(resource_);
		}
		throw std::runtime_error("Resource is not a buffer");
	}

  private:
	std::variant<backend::Buffer *, backend::ImageView *> resource_{};
};

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
