#include "render_resource.h"

namespace xihe::rendering
{

vk::PipelineStageFlags2 get_shader_stage_flags(PassType pass_type)
{
	switch (pass_type)
	{
		case PassType::kCompute:
			return vk::PipelineStageFlagBits2::eComputeShader;

		case PassType::kRaster:
			return vk::PipelineStageFlagBits2::eVertexShader |
			       vk::PipelineStageFlagBits2::eFragmentShader;

		case PassType::kMesh:
			return vk::PipelineStageFlagBits2::eTaskShaderEXT |
			       vk::PipelineStageFlagBits2::eMeshShaderEXT |
			       vk::PipelineStageFlagBits2::eFragmentShader;

		default:
			return {};
	}
}

void update_bindable_state(BindableType type, PassType pass_type, ResourceUsageState &state)
{
	switch (type)
	{
		case BindableType::kSampled:
			state.stage_mask  = get_shader_stage_flags(pass_type);
			state.access_mask = vk::AccessFlagBits2::eShaderRead;
			state.layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
			break;
		case BindableType::kStorageRead:
			state.stage_mask  = get_shader_stage_flags(pass_type);
			state.access_mask = vk::AccessFlagBits2::eShaderRead;
			state.layout      = vk::ImageLayout::eGeneral;
			break;
	}
}

void update_attachment_state(AttachmentType type, ResourceUsageState &state)
{
	switch (type)
	{
		case AttachmentType::kColor:
			state.stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
			state.access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
			state.layout      = vk::ImageLayout::eColorAttachmentOptimal;
			break;
		case AttachmentType::kDepth:
			state.stage_mask = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
			                   vk::PipelineStageFlagBits2::eLateFragmentTests;
			state.access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
			state.layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			break;
	}
}
}        // namespace xihe::rendering
