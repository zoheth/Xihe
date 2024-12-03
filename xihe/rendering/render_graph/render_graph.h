#pragma once
#include "render_resource.h"
#include "backend/command_buffer.h"
#include "backend/sampler.h"
#include "rendering/render_context.h"
#include "rendering/render_target.h"
#include "rendering/passes/render_pass.h"

#include <functional>
#include <variant>

namespace xihe::rendering
{
class GraphBuilder;

using Barrier = std::variant<common::ImageMemoryBarrier, common::BufferMemoryBarrier>;

struct PassInput
{
	ResourceUsage usage;
	std::string        name;
};
struct PassOutput
{
	ResourceUsage                                     usage;
	std::string                                       name;
	vk::Format                                        format;
	vk::Extent3D                                      override_resolution{};
	std::function<vk::Extent3D(const vk::Extent3D &)> modify_extent{};

	vk::AttachmentLoadOp load_op{vk::AttachmentLoadOp::eClear};
	backend::Sampler    *sampler{nullptr};

	void set_sampler(backend::Sampler *s)
	{
		sampler = s;
		usage |= vk::ImageUsageFlagBits::eSampled;
	}
};
struct PassInfo
{
	std::vector<PassInput>  inputs;
	std::vector<PassOutput> outputs;
};


class PassNode
{
  public:
	struct InputInfo
	{
		Barrier barrier;
		std::variant<backend::Buffer*, backend::ImageView*> resource;
	};
	PassNode(std::string name, PassInfo &&pass_info, std::unique_ptr<RenderPass> &&render_pass);

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

	// void add_input_memory_barrier(uint32_t index, Barrier &&barrier);
	void add_input_info(uint32_t index, Barrier &&barrier, std::variant<backend::Buffer*, backend::ImageView*> &&resource);
	void add_output_memory_barrier(uint32_t index, Barrier &&barrier);

	void add_release_barrier(const backend::ImageView *image_view, Barrier &&barrier);

  private:

	std::string name_;

	PassType type_;

	PassInfo pass_info_;

	int64_t batch_index_{-1};

	std::unique_ptr<RenderPass> render_pass_;

	std::unique_ptr<RenderTarget> render_target_;

	// Barriers applied before execution to ensure the input resources are in the correct state for reading.
	std::unordered_map<uint32_t, InputInfo> inputs_;

	// Barriers applied before execution to ensure the output resources are in the correct state for writing.
	std::unordered_map<uint32_t, Barrier> output_barriers_;

	// Barriers applied after execution to release resource ownership for cross-queue synchronization.
	std::unordered_map<const backend::ImageView *, Barrier> release_barriers_;
};

struct PassBatch
{
	std::vector<PassNode *> pass_nodes;
	PassType                type;
	int64_t                 wait_batch_index{-1};
	uint64_t                signal_semaphore_value{0};
};

class RenderGraph
{
  public:
	RenderGraph(RenderContext &render_context);

	void execute();

  private:
	// Called by GraphBuilder
	void add_pass_node(PassNode &&pass_node);

	void execute_raster_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	void execute_compute_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	RenderContext         &render_context_;

	std::vector<PassBatch> pass_batches_{};

	std::vector<PassNode> pass_nodes_{};

	friend GraphBuilder;
};
}        // namespace xihe::rendering
