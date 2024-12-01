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

  private:
	class PassBatchBuilder
	{
	  public:
		void add_pass(RenderPass *pass, int batch_index);

		void add_wait_semaphore(vk::Semaphore semaphore);

		std::vector<PassBatch> finalize();
	private:
		PassInfo current_batch_;
	  std::vector<PassBatch> batches_;
	};

	void build_pass_batches();

	std::pair<std::vector<std::unordered_set<int>>, std::vector<int>>
	    build_dependency_graph();

	void add_pass(const std::string            &name,
	              PassInfo                    &&pass_info,
	              std::unique_ptr<RenderPass> &&render_pass);

	RenderGraph   &render_graph_;
	RenderContext &render_context_;

	bool is_dirty_{false};
};
}        // namespace xihe::rendering
