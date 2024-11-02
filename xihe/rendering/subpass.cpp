#include "subpass.h"

#include "scene_graph/components/light.h"

#include <utility>

namespace xihe
{
const std::vector<std::string> kLightTypeDefinitions = {
    "DIRECTIONAL_LIGHT " + std::to_string(static_cast<float>(sg::LightType::kDirectional)),
    "POINT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::kPoint)),
    "SPOT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::kSpot))};

glm::mat4 xihe::vulkan_style_projection(const glm::mat4 &proj)
{
	// Flip Y in clipspace. X = -1, Y = -1 is topLeft in Vulkan.
	glm::mat4 mat = proj;
	mat[1][1] *= -1;

	return mat;
}
namespace rendering
{
Subpass::Subpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader) :
    render_context_{render_context},
    vertex_shader_{std::move(vertex_shader)},
    fragment_shader_{std::move(fragment_shader)}
{}

Subpass::Subpass(RenderContext &render_context, std::optional<backend::ShaderSource> task_shader, backend::ShaderSource &&mesh_shader, backend::ShaderSource &&fragment_shader) :
    render_context_{render_context},
    task_shader_{std::move(task_shader)},
    mesh_shader_{std::move(mesh_shader)},
    fragment_shader_{std::move(fragment_shader)}
{}

void Subpass::update_render_target_attachments(RenderTarget &render_target) const
{
	render_target.set_input_attachments(input_attachments_);
	render_target.set_output_attachments(output_attachments_);
}

void Subpass::set_render_target(const RenderTarget *render_target)
{
	render_target_ = render_target;
}

void Subpass::draw(backend::CommandBuffer &command_buffer)
{}

RenderContext &Subpass::get_render_context()
{
	return render_context_;
}

const backend::ShaderSource &Subpass::get_vertex_shader() const
{
	if (!vertex_shader_.has_value())
	{
		throw std::runtime_error("Vertex shader not set");
	}

	return vertex_shader_.value();
}

const backend::ShaderSource &Subpass::get_task_shader() const
{
	if (!task_shader_.has_value())
	{
		throw std::runtime_error("Task shader not set");
	}

	return task_shader_.value();
}

const backend::ShaderSource &Subpass::get_mesh_shader() const
{
	if (!mesh_shader_.has_value())
	{
		throw std::runtime_error("Mesh shader not set");
	}

	return mesh_shader_.value();
}

const backend::ShaderSource &Subpass::get_fragment_shader() const
{
	return fragment_shader_;
}

DepthStencilState &Subpass::get_depth_stencil_state()
{
	return depth_stencil_state_;
}

const std::vector<uint32_t> &Subpass::get_input_attachments() const
{
	return input_attachments_;
}

void Subpass::set_input_attachments(std::vector<uint32_t> input)
{
	input_attachments_ = std::move(input);
}

const std::vector<uint32_t> &Subpass::get_output_attachments() const
{
	return output_attachments_;
}

void Subpass::set_output_attachments(std::vector<uint32_t> output)
{
	output_attachments_ = std::move(output);
}

void Subpass::set_sample_count(vk::SampleCountFlagBits sample_count)
{
	sample_count_ = sample_count;
}

const std::vector<uint32_t> &Subpass::get_color_resolve_attachments() const
{
	return color_resolve_attachments_;
}

void Subpass::set_color_resolve_attachments(std::vector<uint32_t> color_resolve)
{
	color_resolve_attachments_ = std::move(color_resolve);
}

const bool &Subpass::get_disable_depth_stencil_attachment() const
{
	return disable_depth_stencil_attachment_;
}

void Subpass::set_disable_depth_stencil_attachment(bool disable_depth_stencil)
{
	disable_depth_stencil_attachment_ = disable_depth_stencil;
}

const uint32_t &Subpass::get_depth_stencil_resolve_attachment() const
{
	return depth_stencil_resolve_attachment_;
}

const uint32_t &Subpass::get_depth_stencil_attachment() const
{
	return depth_stencil_attachment_;
}

void Subpass::set_depth_stencil_attachment(uint32_t depth_attachment)
{
	depth_stencil_attachment_ = depth_attachment;
}

void Subpass::set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve)
{
	depth_stencil_resolve_attachment_ = depth_stencil_resolve;
}

vk::ResolveModeFlagBits Subpass::get_depth_stencil_resolve_mode() const
{
	return depth_stencil_resolve_mode_;
}

void Subpass::set_depth_stencil_resolve_mode(vk::ResolveModeFlagBits mode)
{
	depth_stencil_resolve_mode_ = mode;
}

LightingState &Subpass::get_lighting_state()
{
	return lighting_state_;
}

const std::string &Subpass::get_debug_name() const
{
	return debug_name_;
}

void Subpass::set_debug_name(const std::string &name)
{
	debug_name_ = name;
}

void Subpass::set_thread_index(uint32_t index)
{
	thread_index_ = index;
}
}        // namespace rendering
}        // namespace xihe
