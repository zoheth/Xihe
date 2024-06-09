#pragma once
#include <sstream>
#include <vector>

#include "common/vk_common.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"

namespace xihe::backend
{
class GraphicsPipeline;
class PipelineLayout;
class RenderPass;
class ShaderModule;

enum class ResourceType
{
	kShaderModule,
	kPipelineLayout,
	kRenderPass,
	kGraphicsPipeline
};

class ResourceRecord
{
  public:
	void set_data(const std::vector<uint8_t> &data);

	std::vector<uint8_t> get_data();

	const std::ostringstream &get_stream();

	size_t register_shader_module(vk::ShaderStageFlagBits stage,
	                              const ShaderSource     &glsl_source,
	                              const std::string      &entry_point,
	                              const ShaderVariant    &shader_variant);


	size_t register_pipeline_layout(const std::vector<ShaderModule *> &shader_modules, BindlessDescriptorSet *bindless_descriptor_set = nullptr);

	size_t register_render_pass(const std::vector<rendering::Attachment> &        attachments,
	                            const std::vector<common::LoadStoreInfo>         &load_store_infos,
	                            const std::vector<SubpassInfo>           &        subpasses);

	size_t register_graphics_pipeline(VkPipelineCache pipeline_cache,
	                                  PipelineState  &pipeline_state);

	void set_shader_module(size_t index, const ShaderModule &shader_module);

	void set_pipeline_layout(size_t index, const PipelineLayout &pipeline_layout);

	void set_render_pass(size_t index, const RenderPass &render_pass);

	void set_graphics_pipeline(size_t index, const GraphicsPipeline &graphics_pipeline);

  private:
	std::ostringstream stream_;

	std::vector<size_t> shader_module_indices_;
	std::vector<size_t> pipeline_layout_indices_;
	std::vector<size_t> render_pass_indices_;
	std::vector<size_t> graphics_pipeline_indices_;

	std::unordered_map<const ShaderModule *, size_t>     shader_module_to_index_;
	std::unordered_map<const PipelineLayout *, size_t>   pipeline_layout_to_index_;
	std::unordered_map<const RenderPass *, size_t>       render_pass_to_index_;
	std::unordered_map<const GraphicsPipeline *, size_t> graphics_pipeline_to_index_;
};
}        // namespace xihe::backend
