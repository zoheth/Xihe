#include "resource_record.h"

#include "common/helpers.h"

namespace xihe::backend
{
namespace
{
inline void write_subpass_info(std::ostringstream &os, const std::vector<SubpassInfo> &value)
{
	write(os, value.size());
	for (const SubpassInfo &item : value)
	{
		write(os, item.input_attachments);
		write(os, item.output_attachments);
	}
}

inline void write_processes(std::ostringstream &os, const std::vector<std::string> &value)
{
	write(os, value.size());
	for (const std::string &item : value)
	{
		write(os, item);
	}
}
}        // namespace

void ResourceRecord::set_data(const std::vector<uint8_t> &data)
{
	stream_.str(std::string{data.begin(), data.end()});
}

std::vector<uint8_t> ResourceRecord::get_data()
{
	std::string str = stream_.str();
	return std::vector<uint8_t>{str.begin(), str.end()};
}

const std::ostringstream &ResourceRecord::get_stream()
{
	return stream_;
}

size_t ResourceRecord::register_shader_module(vk::ShaderStageFlagBits stage, const ShaderSource &glsl_source, const std::string &entry_point, const ShaderVariant &shader_variant)
{
	shader_module_indices_.push_back(shader_module_indices_.size());

	write(stream_, ResourceType::kShaderModule, stage, glsl_source.get_source(), entry_point, shader_variant.get_preamble());

	write_processes(stream_, shader_variant.get_processes());

	return shader_module_indices_.back();
}

size_t ResourceRecord::register_pipeline_layout(const std::vector<ShaderModule *> &shader_modules, BindlessDescriptorSet *bindless_descriptor_set)
{
	pipeline_layout_indices_.push_back(pipeline_layout_indices_.size());

	std::vector<size_t> shader_indices(shader_modules.size());
	std::ranges::transform(shader_modules, shader_indices.begin(), [this](const ShaderModule *shader_module) {
		return shader_module_to_index_.at(shader_module);
	});

	write(stream_, ResourceType::kPipelineLayout, shader_indices, bindless_descriptor_set);

	return pipeline_layout_indices_.back();
}

size_t ResourceRecord::register_render_pass(const std::vector<rendering::Attachment> &attachments, const std::vector<common::LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses)
{
	render_pass_indices_.push_back(render_pass_indices_.size());

	write(stream_, ResourceType::kRenderPass, attachments, load_store_infos);
	write_subpass_info(stream_, subpasses);

	return render_pass_indices_.back();
}

size_t ResourceRecord::register_graphics_pipeline(VkPipelineCache pipeline_cache, PipelineState &pipeline_state)
{
	graphics_pipeline_indices_.push_back(graphics_pipeline_indices_.size());

	auto &pipeline_layout = pipeline_state.get_pipeline_layout();
	auto render_pass = pipeline_state.get_render_pass();

	write(stream_, 
		ResourceType::kGraphicsPipeline, 
		pipeline_layout_to_index_.at(&pipeline_layout),
		render_pass_to_index_.at(render_pass),
		pipeline_state.get_subpass_index());

	auto &specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

	write(stream_, specialization_constant_state);

	auto &vertex_input_state = pipeline_state.get_vertex_input_state();

	write(stream_,
	      vertex_input_state.attributes,
	      vertex_input_state.bindings);

	write(stream_,
	      pipeline_state.get_input_assembly_state(),
	      pipeline_state.get_rasterization_state(),
	      pipeline_state.get_viewport_state(),
	      pipeline_state.get_multisample_state(),
	      pipeline_state.get_depth_stencil_state());

	auto &color_blend_state = pipeline_state.get_color_blend_state();

	write(stream_,
	      color_blend_state.logic_op,
	      color_blend_state.logic_op_enable,
	      color_blend_state.attachments);

	return graphics_pipeline_indices_.back();



	return graphics_pipeline_indices_.back();
}

void ResourceRecord::set_shader_module(size_t index, const ShaderModule &shader_module)
{
	shader_module_to_index_[&shader_module] = index;
}

void ResourceRecord::set_pipeline_layout(size_t index, const PipelineLayout &pipeline_layout)
{
	pipeline_layout_to_index_[&pipeline_layout] = index;
}

void ResourceRecord::set_render_pass(size_t index, const RenderPass &render_pass)
{
	render_pass_to_index_[&render_pass] = index;
}

void ResourceRecord::set_graphics_pipeline(size_t index, const GraphicsPipeline &graphics_pipeline)
{
	graphics_pipeline_to_index_[&graphics_pipeline] = index;
}
}        // namespace xihe::backend
