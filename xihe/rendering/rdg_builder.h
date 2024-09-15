#pragma once

#include <TaskScheduler.h>

#include "rdg_pass.h"
#include "render_context.h"

namespace xihe::rendering
{

static void set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent);

struct SecondaryDrawTask : enki::ITaskSet
{
	void init(backend::CommandBuffer *command_buffer, RasterRdgPass const *pass, uint32_t subpass_index);

	void ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum) override;

	backend::CommandBuffer *command_buffer;
	RasterRdgPass const    *pass;
	uint32_t                subpass_index;
};

class RdgBuilder
{
  public:
	explicit RdgBuilder(RenderContext &render_context);

	/*template <typename T, typename... Args>
	void add_pass(std::string name, Args &&...args);*/

	void add_raster_pass(const std::string &name, PassInfo &&pass_info, std::vector<std::unique_ptr<Subpass>> &&subpasses);

	void add_compute_pass(const std::string &name, PassInfo &&pass_info, const std::vector<backend::ShaderSource> &shader_sources, ComputeRdgPass::ComputeFunction &&compute_function);

	void compile();

	void execute();

  private:
	void build_pass_batches();
	void topological_sort();
	void setup_pass_dependencies();

  private:
	struct PassBatch
	{
		std::vector<RdgPass *> passes;
		RdgPassType            type;
		uint64_t               wait_semaphore_value{0};        // 0 meaning no semaphore
		uint64_t               signal_semaphore_value{0};
	};
	RenderContext &render_context_;
	// std::unordered_map<std::string, std::unique_ptr<RdgPass>> rdg_passes_{};
	std::vector<std::unique_ptr<RdgPass>> rdg_passes_{};
	// std::vector<int>                      pass_order_{};
	std::vector<PassBatch> pass_batches_{};

	std::vector<RdgResource> rdg_resources_{};
};

#if 0
template <typename T, typename... Args>
void RdgBuilder::add_pass(std::string name, Args &&...args)
{
	static_assert(std::is_base_of_v<rendering::RdgPass, T>, "T must be a derivative of RenderPass");

	if (rdg_passes_.contains(name))
	{
		throw std::runtime_error{"Pass with name " + name + " already exists"};
	}
	rdg_passes_.emplace(name, std::make_unique<T>(name, render_context_, std::forward<Args>(args)...));

	pass_order_.push_back(name);

	/*if (rdg_passes_[name]->needs_recreate_rt())
	{
	    render_context_.register_rdg_render_target(name, rdg_passes_[name].get());
	}*/
	render_context_.register_rdg_render_target(name, rdg_passes_[name].get());
}
#endif
}        // namespace xihe::rendering
