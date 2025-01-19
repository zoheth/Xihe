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
	return pipeline_stats_data_.contains(index);
}

const StatGraphData &VulkanStatsProvider::get_graph_data(StatIndex index) const
{
	return StatsProvider::get_graph_data(index);
}

StatsProvider::Counters VulkanStatsProvider::sample(float delta_time)
{
	collect_pipeline_stats(graphics_pipeline_stats_);
	Counters out;
	for (auto &[sata_index, value] : pipeline_stats_data_)
	{
		out[sata_index].result = static_cast<double>(value);
		value = 0;
	}

	return out;
}

void VulkanStatsProvider::begin_sampling(backend::CommandBuffer &command_buffer)
{

	uint32_t active_frame_index = render_context_.get_active_frame_index();
	if (command_buffer.is_support_graphics())
	{
		if (graphics_pipeline_stats_.current_query_index >= queries_per_frame_)
		{
			LOGW("Exceeded maximum number of queries per frame");
			return;
		}
		uint32_t query_index = active_frame_index * queries_per_frame_ + graphics_pipeline_stats_.current_query_index;

		if (graphics_pipeline_stats_.query_pool)
		{
			command_buffer.reset_query_pool(*graphics_pipeline_stats_.query_pool, query_index, 1);
			command_buffer.begin_query(*graphics_pipeline_stats_.query_pool, query_index, static_cast<vk::QueryControlFlagBits>(0));
		}	
	}
	else
	{
		
	}
	
}

void VulkanStatsProvider::end_sampling(backend::CommandBuffer &command_buffer)
{
	uint32_t active_frame_index = render_context_.get_active_frame_index();

	if (command_buffer.is_support_graphics())
	{
		uint32_t query_index = active_frame_index * queries_per_frame_ +
		                       graphics_pipeline_stats_.current_query_index;

		if (graphics_pipeline_stats_.current_query_index >= queries_per_frame_)
		{
			LOGW("Exceeded maximum number of queries per frame");
			return;
		}

		if (graphics_pipeline_stats_.query_pool)
		{
			command_buffer.get_handle().pipelineBarrier(
			    vk::PipelineStageFlagBits::eAllCommands,
			    vk::PipelineStageFlagBits::eAllCommands,
			    vk::DependencyFlagBits::eByRegion,
			    {},
			    nullptr,
			    nullptr);

			command_buffer.end_query(*graphics_pipeline_stats_.query_pool, query_index);
		}
		++graphics_pipeline_stats_.current_query_index;
		graphics_pipeline_stats_.frame_query_counts[active_frame_index] = graphics_pipeline_stats_.current_query_index;
	}
	else
	{
		
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

	if (!graphics_pipeline_stats_.query_pool)
	{
		LOGW("Failed to create pipeline stat query pool");
		return false;
	}


	if (has_timestamps_)
	{
		vk::QueryPoolCreateInfo timestamp_pool_create_info;
		timestamp_pool_create_info.queryType  = vk::QueryType::eTimestamp;
		timestamp_pool_create_info.queryCount = num_framebuffers * 2 * queries_per_frame_;        // 2 timestamps per pass batch (start & end)

		timestamp_pool_ = std::make_unique<backend::QueryPool>(device, timestamp_pool_create_info);

		if (!timestamp_pool_)
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
	pipeline_stats.current_query_index = 0;
}
}        // namespace xihe::stats
