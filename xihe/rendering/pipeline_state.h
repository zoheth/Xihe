#pragma once

#include <map>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "backend/pipeline_layout.h"
#include "backend/render_pass.h"
#include "common/helpers.h"

namespace xihe
{
struct VertexInputState
{
	std::vector<vk::VertexInputBindingDescription>   bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
};

struct InputAssemblyState
{
	vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
	bool                  primitive_restart_enable{VK_FALSE};
};

struct RasterizationState
{
	bool depth_clamp_enable{VK_FALSE};
	bool rasterizer_discard_enable{VK_FALSE};

	vk::PolygonMode   polygon_mode{vk::PolygonMode::eFill};
	vk::CullModeFlags cull_mode{vk::CullModeFlagBits::eBack};

	vk::FrontFace front_face{vk::FrontFace::eCounterClockwise};

	vk::Bool32 depth_bias_enable{VK_FALSE};
};

struct ViewportState
{
	uint32_t viewport_count{1};
	uint32_t scissor_count{1};
};

struct MultisampleState
{
	vk::SampleCountFlagBits rasterization_samples{vk::SampleCountFlagBits::e1};
	bool                    sample_shading_enable{VK_FALSE};
	float                   min_sample_shading{1.0f};

	vk::SampleMask sample_mask{0};

	vk::Bool32 alpha_to_coverage_enable{VK_FALSE};
	vk::Bool32 alpha_to_one_enable{VK_FALSE};
};

struct DepthStencilState
{
	bool               depth_test_enable{VK_TRUE};
	bool               depth_write_enable{VK_TRUE};
	vk::CompareOp      depth_compare_op{vk::CompareOp::eGreater};
	bool               depth_bounds_test_enable{VK_FALSE};
	bool               stencil_test_enable{VK_FALSE};
	vk::StencilOpState front{};
	vk::StencilOpState back{};
};

struct ColorBlendAttachmentState
{
	bool blend_enable{VK_FALSE};

	vk::BlendFactor src_color_blend_factor{vk::BlendFactor::eOne};
	vk::BlendFactor dst_color_blend_factor{vk::BlendFactor::eZero};
	vk::BlendOp     color_blend_op{vk::BlendOp::eAdd};
	vk::BlendFactor src_alpha_blend_factor{vk::BlendFactor::eOne};
	vk::BlendFactor dst_alpha_blend_factor{vk::BlendFactor::eZero};
	vk::BlendOp     alpha_blend_op{vk::BlendOp::eAdd};

	vk::ColorComponentFlags color_write_mask{vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
};

struct ColorBlendState
{
	bool        logic_op_enable{VK_FALSE};
	vk::LogicOp logic_op{vk::LogicOp::eClear};

	std::vector<ColorBlendAttachmentState> attachments;
};

/// Helper class to create specialization constants for a Vulkan pipeline. The state tracks a pipeline globally, and not per shader. Two shaders using the same constant_id will have the same data.
class SpecializationConstantState
{
  public:
	void reset();
	bool is_dirty() const;
	void clear_dirty();

	template <class T>
	void set_constant(uint32_t constant_id, const T &value)
	{
		set_constant(constant_id, to_bytes(static_cast<std::uint32_t>(value)));
	}

	void set_constant(uint32_t constant_id, const std::vector<uint8_t> &value);

	void set_specialization_constant_state(const std::map<uint32_t, std::vector<uint8_t>> &state);

	const std::map<uint32_t, std::vector<uint8_t>> &get_specialization_constant_state() const;

  private:
	bool dirty_{false};

	std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state_;
};

class PipelineState
{
  public:
	void reset();

	void set_pipeline_layout(backend::PipelineLayout &pipeline_layout);

	void set_render_pass(const backend::RenderPass &render_pass);

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void set_vertex_input_state(const VertexInputState &vertex_input_state);

	void set_input_assembly_state(const InputAssemblyState &input_assembly_state);

	void set_rasterization_state(const RasterizationState &rasterization_state);

	void set_viewport_state(const ViewportState &viewport_state);

	void set_multisample_state(const MultisampleState &multisample_state);

	void set_depth_stencil_state(const DepthStencilState &depth_stencil_state);

	void set_color_blend_state(const ColorBlendState &color_blend_state);

	void set_subpass_index(uint32_t subpass_index);

	void set_has_mesh_shader(bool has_mesh_shader);

	const backend::PipelineLayout &get_pipeline_layout() const;

	const backend::RenderPass *get_render_pass() const;

	const SpecializationConstantState &get_specialization_constant_state() const;

	const VertexInputState &get_vertex_input_state() const;

	const InputAssemblyState &get_input_assembly_state() const;

	const RasterizationState &get_rasterization_state() const;

	const ViewportState &get_viewport_state() const;

	const MultisampleState &get_multisample_state() const;

	const DepthStencilState &get_depth_stencil_state() const;

	const ColorBlendState &get_color_blend_state() const;

	bool has_mesh_shader() const;

	uint32_t get_subpass_index() const;

	bool is_dirty() const;

	void clear_dirty();

  private:
	bool dirty_{false};

	backend::PipelineLayout *pipeline_layout_{nullptr};

	const backend::RenderPass *render_pass_{nullptr};

	SpecializationConstantState specialization_constant_state_{};
	VertexInputState            vertex_input_state_{};
	InputAssemblyState          input_assembly_state_{};
	RasterizationState          rasterization_state_{};
	ViewportState               viewport_state_{};
	MultisampleState            multisample_state_{};
	DepthStencilState           depth_stencil_state_{};
	ColorBlendState             color_blend_state_{};

	bool has_mesh_shader_{false};

	uint32_t subpass_index_{0U};
};

}        // namespace xihe
