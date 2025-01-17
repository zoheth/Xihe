#pragma once

#include "stats_provider.h"

namespace xihe::stats
{
class VulkanStatsProvider : public StatsProvider
{
private:
	void setup_query_pool();

	// Vector for storing pipeline statistics results
	std::vector<uint64_t>    pipeline_stats_{};
	std::vector<std::string> pipeline_stat_names_{};
};
}