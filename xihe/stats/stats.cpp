#include "stats.h"

#include "backend/allocated.h"
#include "backend/device.h"
#include "rendering/render_context.h"

namespace xihe::stats
{
StatGraphData::StatGraphData(const std::string &name, const std::string &format, float scale_factor, bool has_fixed_max, float max_value) :
    name(name), format(format), scale_factor(scale_factor), has_fixed_max(has_fixed_max), max_value(max_value)
{}

Stats::Stats(rendering::RenderContext &render_context, size_t buffer_size) :
    render_context_(render_context), buffer_size_(buffer_size)
{
	assert(buffer_size >= 2 && "Buffers size should be greater than 2");
}

Stats::~Stats()
{}

void Stats::update(float delta_time)
{
	switch (sampling_config_.mode)
	{
		case CounterSamplingMode::kPolling:
		{
			StatsProvider::Counters sample;

			for (auto &p : providers)
			{
				auto s = p->sample(delta_time);
				sample.insert(s.begin(), s.end());
			}
			push_sample(sample);
			break;
		}
	}
}

void Stats::request_stats(const std::set<StatIndex> &requested_stats, const CounterSamplingConfig &sampling_config)
{
	if (!providers.empty())
	{
		throw std::runtime_error("Stats must only be requested once");
	}

	requested_stats_ = requested_stats;
	sampling_config_ = sampling_config;

	std::set<StatIndex> stats = requested_stats;

	providers.emplace_back(std::make_unique<FrameTimeProvider>(stats));

	for (const auto &stat : requested_stats)
	{
		counters_data_[stat] = std::vector<float>(buffer_size_, 0);
	}

	if (sampling_config.mode == CounterSamplingMode::kContinuous)
	{
		//todo
	}

	for (const auto &stat_index : requested_stats)
	{
		if (!is_available(stat_index))
		{
			LOGW(stats::StatsProvider::get_default_graph_data(stat_index).name + " : not available");
		}
	}
}

const StatGraphData &Stats::get_graph_data(StatIndex index) const
{
	for (const auto &provider : providers)
	{
		if (provider->is_available(index))
		{
			return provider->get_graph_data(index);
		}
	}
	return StatsProvider::get_default_graph_data(index);
}

bool Stats::is_available(StatIndex index) const
{
	return std::ranges::any_of(providers, [index](const auto &provider) {
		return provider->is_available(index);
	});
}

void Stats::push_sample(const StatsProvider::Counters &sample)
{
	for (auto &c : counters_data_)
	{
		StatIndex           idx    = c.first;
		std::vector<float> &values = c.second;

		// Find the counter matching this StatIndex in the Sample
		const auto &smp = sample.find(idx);
		if (smp == sample.end())
		{
			continue;
		}

		auto measurement = static_cast<float>(smp->second.result);

		assert(values.size() >= 2 && "Buffers size should be greater than 2");

		float alpha_smoothing = 0.2f;

		if (values.size() == values.capacity())
		{
			// Shift values to the left to make space at the end and update counters
			std::ranges::rotate(values, values.begin() + 1);
		}

		// Use an exponential moving average to smooth values
		values.back() = measurement * alpha_smoothing + *(values.end() - 2) * (1.0f - alpha_smoothing);
	}
}

void Stats::begin_sampling(backend::CommandBuffer &command_buffer)
{
	for (auto &provider : providers)
	{
		provider->begin_sampling(command_buffer);
	}
}

void Stats::end_sampling(backend::CommandBuffer &command_buffer)
{
	for (auto &provider : providers)
	{
		provider->end_sampling(command_buffer);
	}
}
}        // namespace xihe::stats
