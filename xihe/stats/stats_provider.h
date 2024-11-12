#pragma once
#include <unordered_map>

#include "stats_common.h"

#include <map>

namespace xihe
{
namespace backend
{
class CommandBuffer;
}

namespace stats
{
class StatsProvider
{
  public:
	struct Counter
	{
		double result;
	};

	using Counters = std::unordered_map<StatIndex, Counter, StatIndexHash>;

	virtual ~StatsProvider()
	{}

	virtual bool is_available(StatIndex index) const = 0;

	virtual const StatGraphData &get_graph_data(StatIndex index) const
	{
		return default_graph_map_.at(index);
	}

	static const StatGraphData &get_default_graph_data(StatIndex index);

	virtual Counters sample(float delta_time) = 0;

	virtual Counters continuous_sample(float delta_time)
	{
		return Counters{};
	}

	virtual void begin_sampling(backend::CommandBuffer &command_buffer)
	{}

	virtual void end_sampling(backend::CommandBuffer &command_buffer)
	{}

protected:
	static std::map<StatIndex, StatGraphData> default_graph_map_;
};
}        // namespace stats
}        // namespace xihe
