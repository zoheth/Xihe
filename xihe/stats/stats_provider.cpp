#include "stats_provider.h"

namespace xihe::stats
{
std::map<StatIndex, StatGraphData> StatsProvider::default_graph_map_{
    // clang-format off
	// StatIndex                        Name shown in graph                            Format           Scale                         Fixed_max Max_value
	{StatIndex::kFrameTimes,           {"Frame Times",                                 "{:3.1f} ms",    1.0f}},

	{StatIndex::kInputAssemblyVerts,      {"GPU Input Assembly Vertices",            "{:4.1f}k",      static_cast<float>(1e-3)}},
    {StatIndex::kInputAssemblyPrims,      {"GPU Input Assembly Primitives",          "{:4.1f}k",      static_cast<float>(1e-3)}},
    {StatIndex::kVertexShaderInvocs,      {"GPU Vertex Shader Invocations",          "{:4.1f}k",      static_cast<float>(1e-3)}},
    {StatIndex::kClippingInvocs,          {"GPU Clipper Invocations",               "{:4.1f}k",      static_cast<float>(1e-3)}},
    {StatIndex::kClippingPrims,           {"GPU Clipped Primitives",                "{:4.1f}k",      static_cast<float>(1e-3)}},
    {StatIndex::kFragmentShaderInvocs,    {"GPU Fragment Shader Invocations",        "{:4.1f}k",      static_cast<float>(1e-3)}},
    {StatIndex::kComputeShaderInvocs,     {"GPU Compute Shader Invocations",         "{:4.1f}k",      static_cast<float>(1e-3)}}
    // clang-format on

};

const StatGraphData & StatsProvider::get_default_graph_data(StatIndex index)
{
	return default_graph_map_.at(index);
}
}
