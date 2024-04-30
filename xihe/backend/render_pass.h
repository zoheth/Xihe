#pragma once

#include "common/vk_common.h"
#include "backend/vulkan_resource.h"

namespace xihe
{
namespace rendering
{
struct Attachment;
}
namespace backend
{
struct SubpassInfo
{
	std::vector<uint32_t> input_attachments;
	std::vector<uint32_t> output_attachments;
	std::vector<uint32_t> color_resolve_attachments;

	bool                    disable_depth_stencil_attachment;
	uint32_t                depth_stencil_resolve_attachment;
	vk::ResolveModeFlagBits depth_stencil_resolve_mode;

	std::string debug_name;
};

class RenderPass : public VulkanResource<vk::RenderPass>
{
  public:
	RenderPass(Device                           &device,
	           const std::vector<rendering::Attachment>   &attachments,
	           const std::vector<LoadStoreInfo> &load_store_infos,
	           const std::vector<SubpassInfo>   &subpasses);

  private:
	size_t subpass_count_;
};
}        // namespace backend
}        // namespace xihe