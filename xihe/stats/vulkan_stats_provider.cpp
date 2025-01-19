#include "vulkan_stats_provider.h"

#include "rendering/render_context.h"

#include <vulkan/vulkan.hpp>

namespace xihe::stats
{
void GpuTimeCalculator::add_time_range(uint64_t start, uint64_t end)
{
	if (start > end)
	{
		std::swap(start, end);
	}
	ranges_.push_back({start, end});
}

void GpuTimeCalculator::clear()
{
	ranges_.clear();
}

uint64_t GpuTimeCalculator::calculate_total_time()
{
	if (ranges_.empty())
	{
		return 0;
	}

	std::sort(ranges_.begin(), ranges_.end());

	uint64_t total_time    = 0;
	uint64_t current_start = ranges_[0].start;
	uint64_t current_end   = ranges_[0].end;

	for (size_t i = 1; i < ranges_.size(); ++i)
	{
		if (ranges_[i].start <= current_end)
		{
			current_end = std::max(current_end, ranges_[i].end);
		}
		else
		{
			total_time += (current_end - current_start);
			current_start = ranges_[i].start;
			current_end   = ranges_[i].end;
		}
	}
	total_time += (current_end - current_start);

	return total_time;
}

VulkanStatsProvider::VulkanStatsProvider(std::set<StatIndex> &requested_stats, const CounterSamplingConfig &sampling_config, rendering::RenderContext &render_context) :
    render_context_(render_context)
{
	backend::Device               &device = render_context.get_device();
	const backend::PhysicalDevice &gpu    = device.get_gpu();

	has_timestamps_   = gpu.get_properties().limits.timestampComputeAndGraphics;
	timestamp_period_ = gpu.get_properties().limits.timestampPeriod;

	create_query_pools();
}

VulkanStatsProvider::~VulkanStatsProvider()
{}

bool VulkanStatsProvider::is_available(StatIndex index) const
{
	return pipeline_stats_data_.contains(index) || index == StatIndex::kGpuTime || index == StatIndex::kGraphicsPipelineTime || index == StatIndex::kComputePipelineTime;
}

const StatGraphData &VulkanStatsProvider::get_graph_data(StatIndex index) const
{
	return StatsProvider::get_graph_data(index);
}

StatsProvider::Counters VulkanStatsProvider::sample(float delta_time)
{
	collect_pipeline_stats(graphics_pipeline_stats_);
	collect_pipeline_stats(compute_pipeline_stats_);
	Counters out;
	for (auto &[sata_index, value] : pipeline_stats_data_)
	{
		out[sata_index].result = static_cast<double>(value);
		value                  = 0;
	}

	out[StatIndex::kGraphicsPipelineTime].result = static_cast<double>(graphics_pipeline_stats_.gpu_time);
	out[StatIndex::kComputePipelineTime].result  = static_cast<double>(compute_pipeline_stats_.gpu_time);

	float elapsed_ns                = timestamp_period_ * static_cast<float>(time_calculator_.calculate_total_time());
	out[StatIndex::kGpuTime].result = elapsed_ns * 0.000001f;
	time_calculator_.clear();

	return out;
}

void VulkanStatsProvider::begin_sampling(backend::CommandBuffer &command_buffer)
{
	uint32_t active_frame_index    = render_context_.get_active_frame_index();
	auto     handle_pipeline_stats = [&](PipelineStats &stats) {
        if (stats.current_query_index >= queries_per_frame_)
        {
            LOGW("Exceeded maximum number of queries per frame");
            return;
        }
        uint32_t query_index = active_frame_index * queries_per_frame_ + stats.current_query_index;

        if (stats.query_pool)
        {
            command_buffer.reset_query_pool(*stats.query_pool, query_index, 1);
            command_buffer.begin_query(*stats.query_pool, query_index, static_cast<vk::QueryControlFlagBits>(0));
        }

        if (stats.timestamp_pool)
        {
            uint32_t timestamp_query_index = active_frame_index * queries_per_frame_ * 2 +
                                             stats.current_query_index * 2;

            command_buffer.reset_query_pool(*stats.timestamp_pool, timestamp_query_index, 2);
            command_buffer.write_timestamp(vk::PipelineStageFlagBits::eTopOfPipe,
			                                   *stats.timestamp_pool,
			                                   timestamp_query_index);
        }
	};
	if (command_buffer.is_support_graphics())
	{
		handle_pipeline_stats(graphics_pipeline_stats_);
	}
	else
	{
		handle_pipeline_stats(compute_pipeline_stats_);
	}
}

void VulkanStatsProvider::end_sampling(backend::CommandBuffer &command_buffer)
{
	uint32_t active_frame_index    = render_context_.get_active_frame_index();
	auto     handle_pipeline_stats = [&](PipelineStats &stats) {
        uint32_t query_index = active_frame_index * queries_per_frame_ +
                               stats.current_query_index;
        if (stats.current_query_index >= queries_per_frame_)
        {
            LOGW("Exceeded maximum number of queries per frame");
            return;
        }
        if (stats.query_pool)
        {
            command_buffer.get_handle().pipelineBarrier(
                vk::PipelineStageFlagBits::eAllCommands,
                vk::PipelineStageFlagBits::eAllCommands,
                vk::DependencyFlagBits::eByRegion,
                {},
                nullptr,
                nullptr);
            command_buffer.end_query(*stats.query_pool, query_index);
        }

        if (stats.timestamp_pool)
        {
            uint32_t timestamp_query_index = active_frame_index * queries_per_frame_ * 2 +
                                             stats.current_query_index * 2;
            command_buffer.write_timestamp(vk::PipelineStageFlagBits::eBottomOfPipe,
			                                   *stats.timestamp_pool,
			                                   timestamp_query_index + 1);
        }

        ++stats.current_query_index;
        stats.frame_query_counts[active_frame_index] = stats.current_query_index;
	};

	if (command_buffer.is_support_graphics())
	{
		handle_pipeline_stats(graphics_pipeline_stats_);
	}
	else
	{
		handle_pipeline_stats(compute_pipeline_stats_);
	}
}

bool VulkanStatsProvider::create_query_pools()
{
	graphics_pipeline_stats_.stat_indices = {StatIndex::kInputAssemblyVerts,
	                                         StatIndex::kInputAssemblyPrims,
	                                         StatIndex::kVertexShaderInvocs,
	                                         StatIndex::kClippingInvocs,
	                                         StatIndex::kClippingPrims,
	                                         StatIndex::kFragmentShaderInvocs,
	                                         StatIndex::kComputeShaderInvocs};

	backend::Device               &device           = render_context_.get_device();
	const backend::PhysicalDevice &physical_device  = device.get_gpu();
	uint32_t                       num_framebuffers = static_cast<uint32_t>(render_context_.get_swapchain().get_images().size());

	graphics_pipeline_stats_.frame_query_counts.resize(num_framebuffers, 0);

	{
		vk::QueryPoolCreateInfo query_pool_create_info;
		query_pool_create_info.queryType          = vk::QueryType::ePipelineStatistics;
		query_pool_create_info.pipelineStatistics = vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices |
		                                            vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives |
		                                            vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations |
		                                            vk::QueryPipelineStatisticFlagBits::eClippingInvocations |
		                                            vk::QueryPipelineStatisticFlagBits::eClippingPrimitives |
		                                            vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations |
		                                            vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations;

		query_pool_create_info.queryCount = num_framebuffers * queries_per_frame_;

		graphics_pipeline_stats_.query_pool = std::make_unique<backend::QueryPool>(device, query_pool_create_info);
	}

	if (!graphics_pipeline_stats_.query_pool)
	{
		LOGW("Failed to create graphics pipeline stat query pool");
		return false;
	}

	compute_pipeline_stats_.stat_indices = {StatIndex::kComputeShaderInvocs};
	compute_pipeline_stats_.frame_query_counts.resize(num_framebuffers, 0);

	{
		vk::QueryPoolCreateInfo query_pool_create_info;
		query_pool_create_info.queryType          = vk::QueryType::ePipelineStatistics;
		query_pool_create_info.pipelineStatistics = vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations;

		query_pool_create_info.queryCount = num_framebuffers * queries_per_frame_;

		compute_pipeline_stats_.query_pool = std::make_unique<backend::QueryPool>(device, query_pool_create_info);
	}

	if (!compute_pipeline_stats_.query_pool)
	{
		LOGW("Failed to create compute pipeline stat query pool");
		return false;
	}

	if (has_timestamps_)
	{
		vk::QueryPoolCreateInfo timestamp_pool_create_info;
		timestamp_pool_create_info.queryType  = vk::QueryType::eTimestamp;
		timestamp_pool_create_info.queryCount = num_framebuffers * 2 * queries_per_frame_;        // 2 timestamps per pass batch (start & end)

		graphics_pipeline_stats_.timestamp_pool = std::make_unique<backend::QueryPool>(device, timestamp_pool_create_info);

		compute_pipeline_stats_.timestamp_pool = std::make_unique<backend::QueryPool>(device, timestamp_pool_create_info);

		if (!graphics_pipeline_stats_.timestamp_pool)
		{
			LOGW("Failed to create timestamp query pool");
			return false;
		}
	}

	return true;
}

void VulkanStatsProvider::collect_pipeline_stats(PipelineStats &pipeline_stats)
{
	uint32_t active_frame_index = render_context_.get_active_frame_index();

	uint32_t query_count = pipeline_stats.frame_query_counts[active_frame_index];
	if (!pipeline_stats.query_pool || query_count == 0)
	{
		return;
	}
	uint32_t              base_query = active_frame_index * queries_per_frame_;
	std::vector<uint64_t> result_data(query_count * pipeline_stats.stat_indices.size());

	vk::DeviceSize stride = pipeline_stats.stat_indices.size() * sizeof(uint64_t);

	vk::Result result = pipeline_stats.query_pool->get_results(base_query, query_count, result_data.size() * sizeof(uint64_t), result_data.data(), stride, vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);

	if (result != vk::Result::eSuccess)
	{
		LOGW("Failed to get pipeline stats results");
		return;
	}

	for (uint32_t i = 0; i < query_count; i++)
	{
		for (size_t j = 0; j < pipeline_stats.stat_indices.size(); j++)
		{
			pipeline_stats_data_[pipeline_stats.stat_indices[j]] += result_data[i * pipeline_stats.stat_indices.size() + j];
		}
	}

	if (pipeline_stats.timestamp_pool && has_timestamps_)
	{
		pipeline_stats.gpu_time = 0.0f;
		uint32_t base_query     = active_frame_index * queries_per_frame_ * 2;

		for (uint32_t i = 0; i < query_count; ++i)
		{
			std::array<uint64_t, 2> timestamps;
			vk::Result              result = pipeline_stats.timestamp_pool->get_results(
                base_query + i * 2,
                2,
                sizeof(uint64_t) * 2,
                timestamps.data(),
                sizeof(uint64_t),
                vk::QueryResultFlagBits::e64 |
                    vk::QueryResultFlagBits::eWait);

			if (result == vk::Result::eSuccess)
			{
				float elapsed_ns = timestamp_period_ * static_cast<float>(timestamps[1] - timestamps[0]);
				pipeline_stats.gpu_time += elapsed_ns * 0.000001f;        // to ms

				time_calculator_.add_time_range(timestamps[0], timestamps[1]);
			}
			else
			{
				LOGW("Failed to get timestamp results for query {}", i);
			}
		}
	}

	pipeline_stats.current_query_index = 0;
}
}        // namespace xihe::stats
