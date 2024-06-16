#pragma once

#include "backend/vulkan_resource.h"
#include "common/vk_common.h"

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
	uint32_t                depth_stencil_attachment;			// Used to specify the specific depth attachment number.
	uint32_t                depth_stencil_resolve_attachment;
	vk::ResolveModeFlagBits depth_stencil_resolve_mode;

	std::string debug_name;
};

class RenderPass : public VulkanResource<vk::RenderPass>
{
  public:
	RenderPass(Device                                   &device,
	           const std::vector<rendering::Attachment> &attachments,
	           const std::vector<common::LoadStoreInfo> &load_store_infos,
	           const std::vector<SubpassInfo>           &subpasses);

	RenderPass(const RenderPass &) = delete;

	RenderPass(RenderPass &&other);

	~RenderPass();

	RenderPass &operator=(const RenderPass &) = delete;

	RenderPass &operator=(RenderPass &&) = delete;

	uint32_t get_color_output_count(uint32_t subpass_index) const;

	vk::Extent2D get_render_area_granularity() const;

  private:
	void create_renderpass(const std::vector<rendering::Attachment> &attachments, const std::vector<common::LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses);

  private:
	size_t subpass_count_{};

	std::vector<uint32_t> color_output_count_{};
};
}        // namespace backend
}        // namespace xihe