#pragma once

#include "backend/buffer.h"
#include "common/vk_common.h"
#include "render_resource.h"
#include "rendering/passes/render_pass.h"

#include <variant>

namespace xihe::rendering
{
class RenderGraph;
}

namespace xihe::rendering
{
using Barrier = std::variant<common::ImageMemoryBarrier, common::BufferMemoryBarrier>;

struct PassBindable
{
	BindableType type;
	std::string  name;
};
struct PassAttachment
{
	AttachmentType   type;
	std::string  name;
	vk::Format   format;
	vk::Extent3D extent{};        // 0 means use the swapchain extent.
};
struct PassInfo
{
	std::vector<PassBindable>   bindables;
	std::vector<PassAttachment> attachments;
};

class PassNode
{
  public:
	struct BindableInfo
	{
		ResourceHandle handle;
		Barrier        barrier;
	};
	PassNode(RenderGraph &render_graph, std::string name, PassInfo &&pass_info, std::unique_ptr<RenderPass> &&render_pass);

	void execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, RenderFrame &render_frame);

	PassInfo &get_pass_info();

	PassType get_type() const;

	void set_render_target(std::unique_ptr<RenderTarget> &&render_target);

	/**
	 * \brief
	 * \return If nullptr is returned, it indicates that this pass uses the render target of the render frame
	 */
	RenderTarget *get_render_target();

	void set_batch_index(uint64_t batch_index);

	int64_t get_batch_index() const;

	void add_bindable_info(uint32_t index, const ResourceHandle &handle, Barrier &&barrier);

	void add_attachment_memory_barrier(uint32_t index, Barrier &&barrier);

	void add_release_barrier(const backend::ImageView *image_view, Barrier &&barrier);

  private:
	RenderGraph &render_graph_;

	std::string name_;

	PassType type_;

	PassInfo pass_info_;

	int64_t batch_index_{-1};

	std::unique_ptr<RenderPass> render_pass_;

	std::unique_ptr<RenderTarget> render_target_;

	// Barriers applied before execution to ensure the input resources are in the correct state for reading.
	std::unordered_map<uint32_t, BindableInfo> bindables_;

	// Barriers applied before execution to ensure the output resources are in the correct state for writing.
	std::unordered_map<uint32_t, Barrier> attachment_barriers_;

	// Barriers applied after execution to release resource ownership for cross-queue synchronization.
	std::unordered_map<const backend::ImageView *, Barrier> release_barriers_;
};
}        // namespace xihe::rendering