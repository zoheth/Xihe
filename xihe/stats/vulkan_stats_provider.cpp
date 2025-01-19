#include "vulkan_stats_provider.h"

#include "rendering/render_context.h"

#include <vulkan/vulkan.hpp>

namespace xihe::stats
{
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
	return pipeline_stats_data_.contains(index) || index == StatIndex::kGraphicsPipelineTime || index == StatIndex::kComputePipelineTime;
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
			std::array<uint64_t, 4> timestamps;
			vk::Result              result = pipeline_stats.timestamp_pool->get_results(
                base_query + i * 2,
                2,
                sizeof(uint64_t) * 4,
                timestamps.data(),
                sizeof(uint64_t) * 2,
                vk::QueryResultFlagBits::e64 |
                    vk::QueryResultFlagBits::eWait |
                    vk::QueryResultFlagBits::eWithAvailability);

			if (result == vk::Result::eSuccess && timestamps[1] && timestamps[3])
			{
				float elapsed_ns = timestamp_period_ * static_cast<float>(timestamps[2] - timestamps[0]);
				pipeline_stats.gpu_time += elapsed_ns * 0.000001f;        // to ms
				if (pipeline_stats.gpu_time > 1000)
				{
					LOGE("Pipeline stats query excessive time detected:"
					     "\n    Query index: {}"
					     "\n    Frame index: {}"
					     "\n    Start timestamp: {}"
					     "\n    End timestamp: {}"
					     "\n    Timestamp difference: {}"
					     "\n    Timestamp period: {:.3f}ns"
					     "\n    Elapsed time: {:.3f}ms"
					     "\n    Accumulated GPU time: {:.3f}ms",
					     i,
					     active_frame_index,
					     timestamps[0],
					     timestamps[2],
					     timestamps[2] - timestamps[0],
					     timestamp_period_,
					     elapsed_ns * 0.000001f,
					     pipeline_stats.gpu_time);
				}
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
