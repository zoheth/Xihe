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
		if (pipeline_layout_->get)
	}
}

void PipelineState::set_render_pass(const backend::RenderPass &render_pass)
{}

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
{}

const backend::RenderPass *PipelineState::get_render_pass() const
{}

const SpecializationConstantState &PipelineState::get_specialization_constant_state() const
{}

const VertexInputState &PipelineState::get_vertex_input_state() const
{}

const InputAssemblyState &PipelineState::get_input_assembly_state() const
{}

const RasterizationState &PipelineState::get_rasterization_state() const
{}

const ViewportState &PipelineState::get_viewport_state() const
{}

const MultisampleState &PipelineState::get_multisample_state() const
{}

const DepthStencilState &PipelineState::get_depth_stencil_state() const
{}

const ColorBlendState &PipelineState::get_color_blend_state() const
{}

uint32_t PipelineState::get_subpass_index() const
{}

bool PipelineState::is_dirty() const
{}

void PipelineState::clear_dirty()
{}
}        // namespace xihe
