#pragma once

#include "stats_provider.h"

namespace xihe::stats
{
class FrameTimeProvider : public StatsProvider
{
public:
	FrameTimeProvider(std::set<StatIndex> &requested_stats)
	{
		// Remove from requested set to stop other providers looking for it.
		requested_stats.erase(StatIndex::kFrameTimes);
	
	}

	bool is_available(StatIndex index) const override
	{
		return index == StatIndex::kFrameTimes;
	}

	Counters sample(float delta_time) override
	{
		Counters res;
		// frame_times comes directly from delta_time
		res[StatIndex::kFrameTimes].result = delta_time * 1000.f;
		return res;
	}
};
}
