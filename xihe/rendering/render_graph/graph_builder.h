#pragma once

#include "backend/sampler.h"
#include "backend/shader_module.h"
#include "render_graph.h"
#include "render_resource.h"
#include "rendering/passes/render_pass.h"

#include <functional>
#include <optional>

namespace xihe::rendering
{

class ResourceStateTracker
{
  public:
	struct State
	{
		ResourceUsageState      usage_state;
		int32_t                 producer_pass{-1};
		int32_t                 last_user{-1};
		backend::ImageView     *image_view{nullptr};
	};

	void track_resource(PassNode &pass_node, int32_t pass_idx);

	void track_resource(const std::string &name, const ResourceUsageState &state);

  private:
	std::unordered_map<std::string, State> states_;
};

class GraphBuilder
{
  public:
	class PassBuilder
	{
	  public:
		PassBuilder(GraphBuilder &graph_builder, std::string name, std::unique_ptr<RenderPass> &&render_pass);

		PassBuilder &inputs(std::initializer_list<PassInput> inputs);

		PassBuilder &outputs(std::initializer_list<PassOutput> outputs);

		PassBuilder &shader(std::initializer_list<std::string> file_names);

		void finalize();

	  private:
		GraphBuilder               &graph_builder_;
		std::string                 pass_name_;
		PassInfo                    pass_info_;
		std::unique_ptr<RenderPass> render_pass_;
	};

	GraphBuilder(RenderGraph &render_graph, RenderContext &render_context) :
	    render_graph_(render_graph), render_context_(render_context)
	{}

	PassBuilder add_pass(const std::string &name, std::unique_ptr<RenderPass> &&render_pass)
	{
		return {*this, name, std::move(render_pass)};
	}
	void build();

	class PassBatchBuilder
	{
	  public:
		void process_pass(PassNode *pass);

		void set_batch_dependency(int64_t wait_batch_index);

		std::vector<PassBatch> finalize();

	  private:
		void finalize_current_batch();

		PassBatch              current_batch_;
		std::vector<PassBatch> batches_;
	};

	void add_pass(const std::string            &name,
	              PassInfo                    &&pass_info,
	              std::unique_ptr<RenderPass> &&render_pass);

	void build_pass_batches();

	std::pair<std::vector<std::unordered_set<uint32_t>>, std::vector<uint32_t>>
	    build_dependency_graph() const;

	void process_pass_inputs(
	    uint32_t              node,
	    PassNode             &current_pass,
	    ResourceStateTracker &resource_tracker,
	    PassBatchBuilder     &batch_builder);

	void process_pass_outputs(
	    uint32_t              node,
	    PassNode             &current_pass,
	    ResourceStateTracker &resource_tracker);

	RenderGraph   &render_graph_;
	RenderContext &render_context_;

	bool is_dirty_{false};
};
}        // namespace xihe::rendering
