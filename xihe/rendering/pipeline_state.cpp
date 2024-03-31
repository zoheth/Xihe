#include "pipeline_state.h"

namespace xihe
{
void SpecializationConstantState::reset()
{
	if (dirty_)
	{
		specialization_constant_state_.clear();
	}

	dirty_ = false;
}

bool SpecializationConstantState::is_dirty() const
{
	return dirty_;
}

void SpecializationConstantState::clear_dirty()
{
	dirty_ = false;
}

void SpecializationConstantState::set_constant(uint32_t constant_id, const std::vector<uint8_t> &value)
{
	const auto data = specialization_constant_state_.find(constant_id);

	if (data != specialization_constant_state_.end() && data->second == value)
	{
		return;
	}

	dirty_ = true;

	specialization_constant_state_[constant_id] = value;
}

void SpecializationConstantState::set_specialization_constant_state(const std::map<uint32_t, std::vector<uint8_t>> &state)
{
	specialization_constant_state_ = state;
}

const std::map<uint32_t, std::vector<uint8_t>> &SpecializationConstantState::get_specialization_constant_state() const
{
	return specialization_constant_state_;
}

void PipelineState::reset()
{
	clear_dirty();

	pipeline_layout_ = nullptr;
	render_pass_     = nullptr;

	specialization_constant_state_.reset();

	vertex_input_state_   = {};
	input_assembly_state_ = {};
	rasterization_state_  = {};
	multisample_state_    = {};
	depth_stencil_state_  = {};
	color_blend_state_    = {};
	subpass_index_        = {0U};
}

void PipelineState::set_pipeline_layout(backend::PipelineLayout &pipeline_layout)
{
	if (pipeline_layout_)
	{
		if (pipeline_layout_->get_handle() != pipeline_layout.get_handle())
		{
			pipeline_layout_ = &pipeline_layout;

			dirty_ = true;
		}
	}
	else
	{
		pipeline_layout_ = &pipeline_layout;

		dirty_ = true;
	}
}

void PipelineState::set_render_pass(const backend::RenderPass &render_pass)
{
	if (render_pass_)
	{
		if (render_pass_->get_handle() != render_pass.get_handle())
		{
			render_pass_ = &render_pass;

			dirty_ = true;
		}
	}
	else
	{
		render_pass_ = &render_pass;

		dirty_ = true;
	}
}

void PipelineState::set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data)
{}

void PipelineState::set_vertex_input_state(const VertexInputState &vertex_input_state)
{}

void PipelineState::set_input_assembly_state(const InputAssemblyState &input_assembly_state)
{}

void PipelineState::set_rasterization_state(const RasterizationState &rasterization_state)
{}

void PipelineState::set_viewport_state(const ViewportState &viewport_state)
{}

void PipelineState::set_multisample_state(const MultisampleState &multisample_state)
{}

void PipelineState::set_depth_stencil_state(const DepthStencilState &depth_stencil_state)
{}

void PipelineState::set_color_blend_state(const ColorBlendState &color_blend_state)
{}

void PipelineState::set_subpass_index(uint32_t subpass_index)
{}

const backend::PipelineLayout &PipelineState::get_pipeline_layout() const
{
	assert(pipeline_layout_ && "Graphics state Pipeline layout is not set");
	return *pipeline_layout_;
}

const backend::RenderPass *PipelineState::get_render_pass() const
{
	return render_pass_;
}

const SpecializationConstantState &PipelineState::get_specialization_constant_state() const
{
	return specialization_constant_state_;
}

const VertexInputState &PipelineState::get_vertex_input_state() const
{
	return vertex_input_state_;
}

const InputAssemblyState &PipelineState::get_input_assembly_state() const
{
	return input_assembly_state_;
}

const RasterizationState &PipelineState::get_rasterization_state() const
{
	return rasterization_state_;
}

const ViewportState &PipelineState::get_viewport_state() const
{
	return viewport_state_;
}

const MultisampleState &PipelineState::get_multisample_state() const
{
	return multisample_state_;
}

const DepthStencilState &PipelineState::get_depth_stencil_state() const
{
	return depth_stencil_state_;
}

const ColorBlendState &PipelineState::get_color_blend_state() const
{
	return color_blend_state_;
}

uint32_t PipelineState::get_subpass_index() const
{
	return subpass_index_;
}

bool PipelineState::is_dirty() const
{
	return dirty_ || specialization_constant_state_.is_dirty();
}

void PipelineState::clear_dirty()
{
	dirty_ = false;
	specialization_constant_state_.clear_dirty();
}
}        // namespace xihe
