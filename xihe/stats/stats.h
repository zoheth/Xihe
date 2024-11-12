#pragma once

#include <future>
#include <map>
#include <set>
#include <vector>

#include "common/timer.h"
#include "stats/frame_time_provider.h"
#include "stats/stats_common.h"
#include "stats_provider.h"

namespace xihe
{
namespace backend
{
class Device;
class CommandBuffer;
}        // namespace backend

namespace rendering
{
class RenderContext;
}

namespace stats
{

class Stats
{
  public:
	explicit Stats(rendering::RenderContext &render_context, size_t buffer_size = 16);

	~Stats();

	void update(float delta_time);

	void request_stats(const std::set<StatIndex> &requested_stats, const CounterSamplingConfig &sampling_config = {CounterSamplingMode::kPolling});

	const StatGraphData &get_graph_data(StatIndex index) const;

	const std::vector<float> &get_data(StatIndex index) const
	{
		return counters_data_.at(index);
	}

	const std::set<StatIndex> &get_requested_stats() const
	{
		return requested_stats_;
	}

	bool is_available(StatIndex index) const;

	void push_sample(const StatsProvider::Counters &sample);

  private:
	rendering::RenderContext &render_context_;

	std::set<StatIndex> requested_stats_;

	std::vector<std::unique_ptr<StatsProvider>> providers;

	CounterSamplingConfig sampling_config_;

	size_t buffer_size_;

	/// Circular buffers for counter data
	std::map<StatIndex, std::vector<float>> counters_data_;
};
}        // namespace stats
}        // namespace xihe
