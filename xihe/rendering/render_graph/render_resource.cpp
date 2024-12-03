#include "render_resource.h"

namespace xihe::rendering
{
bool is_image_resource(ResourceUsage usage)
{
	return usage == ResourceUsage::kSampledImage ||
	       usage == ResourceUsage::kStorageImageRead ||
	       usage == ResourceUsage::kStorageImageWrite ||
	       usage == ResourceUsage::kStorageImageReadWrite ||
	       usage == ResourceUsage::kColorAttachment ||
	       usage == ResourceUsage::kDepthStencilAttachment;
}

void update_resource_state(ResourceUsage usage, PassType pass_type, ResourceUsageState &state)
{
	// Reset masks
	state.stage_mask  = {};
	state.access_mask = {};

	switch (pass_type)
	{
		case PassType::kRaster:
			switch (usage)
			{
				case ResourceUsage::kVertexBuffer:
					state.stage_mask  = vk::PipelineStageFlagBits2::eVertexInput;
					state.access_mask = vk::AccessFlagBits2::eVertexAttributeRead;
					break;

				case ResourceUsage::kIndexBuffer:
					state.stage_mask  = vk::PipelineStageFlagBits2::eIndexInput;
					state.access_mask = vk::AccessFlagBits2::eIndexRead;
					break;

				case ResourceUsage::kUniformBuffer:
					state.stage_mask = vk::PipelineStageFlagBits2::eVertexShader |
					                   vk::PipelineStageFlagBits2::eFragmentShader;
					state.access_mask = vk::AccessFlagBits2::eUniformRead;
					break;

				case ResourceUsage::kSampledImage:
					state.stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;
					state.access_mask = vk::AccessFlagBits2::eShaderRead;
					break;

				case ResourceUsage::kColorAttachment:
					state.stage_mask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
					state.access_mask = vk::AccessFlagBits2::eColorAttachmentWrite;
					break;

				case ResourceUsage::kDepthStencilAttachment:
					state.stage_mask = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
					                   vk::PipelineStageFlagBits2::eLateFragmentTests;
					state.access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
					                    vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
					break;

				default:
					// Handle invalid usage in raster pass
					break;
			}
			break;

		case PassType::kCompute:
			state.stage_mask = vk::PipelineStageFlagBits2::eComputeShader;
			switch (usage)
			{
				case ResourceUsage::kUniformBuffer:
					state.access_mask = vk::AccessFlagBits2::eUniformRead;
					break;

				case ResourceUsage::kStorageBufferRead:
					state.access_mask = vk::AccessFlagBits2::eShaderStorageRead;
					break;

				case ResourceUsage::kStorageBufferWrite:
					state.access_mask = vk::AccessFlagBits2::eShaderStorageWrite;
					break;

				case ResourceUsage::kStorageBufferReadWrite:
					state.access_mask = vk::AccessFlagBits2::eShaderStorageRead |
					                    vk::AccessFlagBits2::eShaderStorageWrite;
					break;

				case ResourceUsage::kStorageImageRead:
					state.access_mask = vk::AccessFlagBits2::eShaderRead;
					break;

				case ResourceUsage::kStorageImageWrite:
					state.access_mask = vk::AccessFlagBits2::eShaderWrite;
					break;

				case ResourceUsage::kStorageImageReadWrite:
					state.access_mask = vk::AccessFlagBits2::eShaderRead |
					                    vk::AccessFlagBits2::eShaderWrite;
					break;

				default:
					// Handle invalid usage in compute pass
					break;
			}
			break;

		case PassType::kMesh:
			// Similar to raster pass but with mesh shader stage flags
			break;

		case PassType::kNone:
			// Handle invalid pass type
			break;
	}

	// Update image layout for image resources
	if (is_image_resource(usage))
	{
		switch (usage)
		{
			case ResourceUsage::kSampledImage:
				state.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				break;

			case ResourceUsage::kStorageImageRead:
			case ResourceUsage::kStorageImageWrite:
			case ResourceUsage::kStorageImageReadWrite:
				state.layout = vk::ImageLayout::eGeneral;
				break;

			case ResourceUsage::kColorAttachment:
				state.layout = vk::ImageLayout::eColorAttachmentOptimal;
				break;

			case ResourceUsage::kDepthStencilAttachment:
				state.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				break;

			default:
				break;
		}
	}
}
}
