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
	struct StatData
	{};
	using StatDataMap = std::unordered_map<StatIndex, StatData, StatIndexHash>;

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
	bool create_query_pools(uint32_t queue_family_index);

	rendering::RenderContext &render_context_;

	std::unique_ptr<backend::QueryPool> query_pool_{};

	// An ordered list of the Vulkan counter ids
	std::vector<uint32_t> counter_indices_;

	// Vector for storing pipeline statistics results
	std::vector<uint64_t>  pipeline_stats_{};
	std::vector<StatIndex> stat_indices_{};

	// How many queries have been ended?
	uint32_t queries_ready_ = 0;
};
}        // namespace stats
}        // namespace xihe
