#pragma once
#include "backend/command_buffer.h"
#include "backend/sampler.h"
#include "pass_node.h"
#include "render_resource.h"
#include "rendering/passes/render_pass.h"
#include "rendering/render_context.h"
#include "rendering/render_target.h"

#include <functional>
#include <variant>

namespace xihe::rendering
{
class GraphBuilder;

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

	~RenderGraph() = default;

	void execute();

	ShaderBindable get_resource_bindable(ResourceHandle handle) const;

  private:
	// Called by GraphBuilder
	void add_pass_node(PassNode &&pass_node);

	void execute_raster_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	void execute_compute_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	RenderContext &render_context_;

	std::vector<PassBatch> pass_batches_{};

	std::vector<PassNode> pass_nodes_{};

	std::unordered_map<ResourceHandle, ResourceInfo> resources_{};

	// must use unique_ptr to avoid address invalidation
	std::vector<std::unique_ptr<backend::Image>>     images_;
	std::vector<std::unique_ptr<backend::Buffer>>    buffers_;
	std::vector<std::unique_ptr<backend::ImageView>> image_views_;

	friend GraphBuilder;
};
}        // namespace xihe::rendering
