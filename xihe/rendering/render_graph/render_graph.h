#pragma once
#include "render_resource.h"
#include "pass_node.h"
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

	ShaderBindable get_resource_bindable(ResourceHandle handle) const;

  private:
	// Called by GraphBuilder
	void add_pass_node(PassNode &&pass_node);

	void execute_raster_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	void execute_compute_batch(PassBatch &pass_batch, bool is_first, bool is_last);

	RenderContext         &render_context_;

	std::vector<PassBatch> pass_batches_{};

	std::vector<PassNode> pass_nodes_{};

	std::unordered_map<ResourceHandle, ResourceInfo> resources_{};

	friend GraphBuilder;
};
}        // namespace xihe::rendering
