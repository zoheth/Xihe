#include "vulkan_stats_provider.h"

#include "rendering/render_context.h"

#include <vulkan/vulkan.hpp>

namespace xihe::stats
{
VulkanStatsProvider::VulkanStatsProvider(std::set<StatIndex> &requested_stats, const CounterSamplingConfig &sampling_config, rendering::RenderContext &render_context) :
    render_context_(render_context)
{
	backend::Device  &device             = render_context.get_device();
	uint32_t         queue_family_index = device.get_queue_family_index(vk::QueueFlagBits::eGraphics);
	create_query_pools(queue_family_index);
}

VulkanStatsProvider::~VulkanStatsProvider()
{}

bool VulkanStatsProvider::is_available(StatIndex index) const
{
	for (const auto &stat_index : stat_indices_)
	{
		if (stat_index == index)
		{
			return true;
		}
	}
	return false;
}

const StatGraphData &VulkanStatsProvider::get_graph_data(StatIndex index) const
{
	return StatsProvider::get_graph_data(index);
}

StatsProvider::Counters VulkanStatsProvider::sample(float delta_time)
{
	Counters out;
	if (!query_pool_ || queries_ready_ == 0)
	{
		return out;
	}

	uint32_t active_frame_index = render_context_.get_active_frame_index();

	vk::DeviceSize data_size = pipeline_stats_.size() * sizeof(uint64_t);
	vk::DeviceSize stride    = stat_indices_.size() * sizeof(uint64_t);

	vk::Result result = query_pool_->get_results(active_frame_index, 1, data_size, pipeline_stats_.data(), stride, vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);

	if (result != vk::Result::eSuccess)
	{
		LOGW("Failed to get pipeline stats results");
		return out;
	}

	for (size_t i = 0; i < stat_indices_.size(); ++i)
	{
		out[stat_indices_[i]].result = static_cast<double>(pipeline_stats_[i]);
	}

	queries_ready_ = 0;

	return out;
}

void VulkanStatsProvider::begin_sampling(backend::CommandBuffer &command_buffer)
{
	uint32_t active_frame_index = render_context_.get_active_frame_index();

	if (query_pool_)
	{
		command_buffer.begin_query(*query_pool_, active_frame_index, static_cast<vk::QueryControlFlagBits>(0));
	}
}

void VulkanStatsProvider::end_sampling(backend::CommandBuffer &command_buffer)
{
	uint32_t active_frame_index = render_context_.get_active_frame_index();

	if (query_pool_)
	{
		command_buffer.get_handle().pipelineBarrier(
		    vk::PipelineStageFlagBits::eAllCommands,
		    vk::PipelineStageFlagBits::eAllCommands,
		    vk::DependencyFlagBits::eByRegion,
		    {},
		    nullptr,
		    nullptr);

		command_buffer.end_query(*query_pool_, active_frame_index);

		queries_ready_ = 1;
	}
}

bool VulkanStatsProvider::create_query_pools(uint32_t queue_family_index)
{
	stat_indices_ = {StatIndex::kInputAssemblyVerts,
	                 StatIndex::kInputAssemblyPrims,
	                 StatIndex::kVertexShaderInvocs,
	                 StatIndex::kClippingInvocs,
	                 StatIndex::kClippingPrims,
	                 StatIndex::kFragmentShaderInvocs,
	                 StatIndex::kComputeShaderInvocs};
	pipeline_stats_.resize(stat_indices_.size());

	backend::Device               &device           = render_context_.get_device();
	const backend::PhysicalDevice &physical_device  = device.get_gpu();
	uint32_t                       num_framebuffers = static_cast<uint32_t>(render_context_.get_swapchain().get_images().size());

	vk::QueryPoolCreateInfo query_pool_create_info;
	query_pool_create_info.queryType          = vk::QueryType::ePipelineStatistics;
	query_pool_create_info.pipelineStatistics = vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices |
	                                            vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives |
	                                            vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations |
	                                            vk::QueryPipelineStatisticFlagBits::eClippingInvocations |
	                                            vk::QueryPipelineStatisticFlagBits::eClippingPrimitives |
	                                            vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations |
	                                            vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations;

	query_pool_create_info.queryCount = num_framebuffers;

	query_pool_ = std::make_unique<backend::QueryPool>(device, query_pool_create_info);

	if (!query_pool_)
	{
		LOGW("Failed to create pipeline stat query pool");
		return false;
	}

	query_pool_->host_reset(0, num_framebuffers);

	return true;
}
}        // namespace xihe::stats
