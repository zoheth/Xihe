#pragma once

#include <set>

#include "backend/query_pool.h"
#include "stats_provider.h"

namespace xihe
{
namespace rendering
{
class RenderContext;
}

namespace stats
{
class VulkanStatsProvider : public StatsProvider
{
	struct PipelineStats
	{
		std::unique_ptr<backend::QueryPool> query_pool;
		std::vector<StatIndex>              stat_indices;
		uint32_t                            current_query_index{0};
		std::vector<uint32_t>               frame_query_counts;

		std::unique_ptr<backend::QueryPool> timestamp_pool;

		float gpu_time{0.0f};
	};

  public:
	VulkanStatsProvider(std::set<StatIndex> &requested_stats, const CounterSamplingConfig &sampling_config,
	                    rendering::RenderContext &render_context);

	~VulkanStatsProvider();

	bool is_available(StatIndex index) const override;

	const StatGraphData &get_graph_data(StatIndex index) const override;

	Counters sample(float delta_time) override;

	void begin_sampling(backend::CommandBuffer &command_buffer) override;

	void end_sampling(backend::CommandBuffer &command_buffer) override;

  private:
	bool create_query_pools();

	void collect_pipeline_stats(PipelineStats &pipeline_stats);

	// Maximum number of pass batches that can be tracked per frame
	static constexpr uint32_t queries_per_frame_ = 8;

	rendering::RenderContext &render_context_;

	PipelineStats graphics_pipeline_stats_{};
	PipelineStats compute_pipeline_stats_{};

	bool  has_timestamps_{false};
	float timestamp_period_{1.0f};

	// An ordered list of the Vulkan counter ids
	std::vector<uint32_t> counter_indices_;

	// Vector for storing pipeline statistics results
	std::unordered_map<StatIndex, uint64_t, StatIndexHash> pipeline_stats_data_{};
};
}        // namespace stats
}        // namespace xihe
