#pragma once

#include <chrono>
#include <string>

namespace xihe::stats
{
enum class StatIndex
{
	kFrameTimes,
	kCpuCycles,
	kCpuInstructions,
	kCpuCacheMissRatio,
	kCpuBranchMissRatio,
	kCpuL1Accesses,
	kCpuInstrRetired,
	kCpuL2Accesses,
	kCpuL3Accesses,
	kCpuBusReads,
	kCpuBusWrites,
	kCpuMemReads,
	kCpuMemWrites,
	kCpuAseSpec,
	kCpuVfpSpec,
	kCpuCryptoSpec,

	kGpuCycles,
	kGpuVertexCycles,
	kGpuLoadStoreCycles,
	kGpuTiles,
	kGpuKilledTiles,
	kGpuFragmentJobs,
	kGpuFragmentCycles,
	kGpuExtReads,
	kGpuExtWrites,
	kGpuExtReadStalls,
	kGpuExtWriteStalls,
	kGpuExtReadBytes,
	kGpuExtWriteBytes,
	kGpuTexCycles,
};

struct StatIndexHash
{
	template <typename T>
	size_t operator()(T t) const
	{
		return static_cast<size_t>(t);
	}
};

enum class StatScaling
{
	kNone,

	// The stat is scaled by delta time, useful for per-second values
	kByDeltaTime,

	// The stat is scaled by another counter, useful for ratios
	kByCounter
};

enum class CounterSamplingMode
{
	// Sample counters only when calling update()
	kPolling,

	// Sample counters continuously, update circular buffers when calling update()
	kContinuous
};

struct CounterSamplingConfig
{
	CounterSamplingMode mode;

	std::chrono::milliseconds interval_ms{1};

	float speed{0.5f};
};

struct StatGraphData
{
	StatGraphData(const std::string &name,
	              const std::string &format,
	              float              scale_factor  = 1.0f,
	              bool               has_fixed_max = false,
	              float              max_value     = 0.0f);

	StatGraphData() = default;

	std::string name;
	std::string format;
	float       scale_factor;
	bool        has_fixed_max;
	float       max_value;
};
}