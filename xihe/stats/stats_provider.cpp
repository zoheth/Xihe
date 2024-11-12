#include "stats_provider.h"

namespace xihe::stats
{
std::map<StatIndex, StatGraphData> StatsProvider::default_graph_map_{
    // clang-format off
	// StatIndex                        Name shown in graph                            Format           Scale                         Fixed_max Max_value
	{StatIndex::kFrameTimes,           {"Frame Times",                                 "{:3.1f} ms",    1000.0f}},
	{StatIndex::kCpuCycles,            {"CPU Cycles",                                  "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuInstructions,      {"CPU Instructions",                            "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuCacheMissRatio,    {"Cache Miss Ratio",                            "{:3.1f}%",      100.0f,                       true,     100.0f}},
	{StatIndex::kCpuBranchMissRatio,   {"Branch Miss Ratio",                           "{:3.1f}%",      100.0f,                       true,     100.0f}},
	{StatIndex::kCpuL1Accesses,        {"CPU L1 Accesses",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuInstrRetired,      {"CPU Instructions Retired",                    "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuL2Accesses,        {"CPU L2 Accesses",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuL3Accesses,        {"CPU L3 Accesses",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuBusReads,          {"CPU Bus Read Beats",                          "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuBusWrites,         {"CPU Bus Write Beats",                         "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuMemReads,          {"CPU Memory Read Instructions",                "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuMemWrites,         {"CPU Memory Write Instructions",               "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuAseSpec,           {"CPU Speculatively Exec. SIMD Instructions",   "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuVfpSpec,           {"CPU Speculatively Exec. FP Instructions",     "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kCpuCryptoSpec,        {"CPU Speculatively Exec. Crypto Instructions", "{:4.1f} M/s",   static_cast<float>(1e-6)}},

	{StatIndex::kGpuCycles,            {"GPU Cycles",                                  "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuVertexCycles,      {"Vertex Cycles",                               "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuLoadStoreCycles,   {"Load Store Cycles",                           "{:4.0f} k/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuTiles,             {"Tiles",                                       "{:4.1f} k/s",   static_cast<float>(1e-3)}},
	{StatIndex::kGpuKilledTiles,       {"Tiles killed by CRC match",                   "{:4.1f} k/s",   static_cast<float>(1e-3)}},
	{StatIndex::kGpuFragmentJobs,      {"Fragment Jobs",                               "{:4.0f}/s"}},
	{StatIndex::kGpuFragmentCycles,    {"Fragment Cycles",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuTexCycles,         {"Shader Texture Cycles",                       "{:4.0f} k/s",   static_cast<float>(1e-3)}},
	{StatIndex::kGpuExtReads,          {"External Reads",                              "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuExtWrites,         {"External Writes",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuExtReadStalls,     {"External Read Stalls",                        "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuExtWriteStalls,    {"External Write Stalls",                       "{:4.1f} M/s",   static_cast<float>(1e-6)}},
	{StatIndex::kGpuExtReadBytes,      {"External Read Bytes",                         "{:4.1f} MiB/s", 1.0f / (1024.0f * 1024.0f)}},
	{StatIndex::kGpuExtWriteBytes,     {"External Write Bytes",                        "{:4.1f} MiB/s", 1.0f / (1024.0f * 1024.0f)}},
    // clang-format on

};

const StatGraphData & StatsProvider::get_default_graph_data(StatIndex index)
{
	return default_graph_map_.at(index);
}
}
