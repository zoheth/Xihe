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
	State get_or_create_state(const std::string &name);

	void track_resource(const std::string &name, uint32_t node, const ResourceUsageState &state);

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

		PassBuilder &bindables(std::initializer_list<PassBindable> bindables);

		PassBuilder &attachments(std::initializer_list<PassAttachment> attachments);

		PassBuilder &shader(std::initializer_list<std::string> file_names);

		PassBuilder &present();

		void finalize();

	  private:
		GraphBuilder               &graph_builder_;
		std::string                 pass_name_;
		PassInfo                    pass_info_;
		std::unique_ptr<RenderPass> render_pass_;
		bool                        is_present_{false};
	};

	GraphBuilder(RenderGraph &render_graph, RenderContext &render_context) :
	    render_graph_(render_graph), render_context_(render_context)
	{}

	PassBuilder add_pass(const std::string &name, std::unique_ptr<RenderPass> &&render_pass)
	{
		return {*this, name, std::move(render_pass)};
	}
	void build();

private:

	class PassBatchBuilder
	{
	  public:
		void process_pass(PassNode *pass);

		void set_batch_dependency(int64_t wait_batch_index);

		std::vector<PassBatch> finalize();

	  private:
		void finalize_current_batch();

		PassBatch              current_batch_{};
		std::vector<PassBatch> batches_;
	};

	void add_pass(const std::string            &name,
	              PassInfo                    &&pass_info,
	              std::unique_ptr<RenderPass> &&render_pass,
	              bool is_present);

	void create_resources();

	void build_pass_batches();

	std::pair<std::vector<std::unordered_set<uint32_t>>, std::vector<uint32_t>>
	    build_dependency_graph() const;

	void process_pass_resources(
	    uint32_t              node,
	    PassNode             &pass,
	    ResourceStateTracker &tracker,
	    PassBatchBuilder     &batch_builder);


	RenderGraph   &render_graph_;
	RenderContext &render_context_;

	std::vector<backend::Image> images_;
	std::vector<backend::Buffer> buffers_;
	std::vector<backend::ImageView> image_views_;

	std::unordered_map<std::string, ResourceHandle> resource_handles_;

	bool is_dirty_{false};
};
}        // namespace xihe::rendering
