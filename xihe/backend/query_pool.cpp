#include "query_pool.h"

#include "device.h"

namespace xihe::backend
{
QueryPool::QueryPool(Device &device, vk::QueryPoolCreateInfo &info) :
    device_{device}
{
	auto result = device_.get_handle().createQueryPool(&info, nullptr, &handle_);
	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error{"Failed to create query pool"};
	}
}

QueryPool::QueryPool(QueryPool &&pool):
	device_{pool.device_},
	handle_{pool.handle_}
{
	pool.handle_ = VK_NULL_HANDLE;
}

QueryPool::~QueryPool()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		device_.get_handle().destroyQueryPool(handle_, nullptr);
	}

}

vk::QueryPool QueryPool::get_handle() const
{
	assert(handle_ != VK_NULL_HANDLE && "QueryPool handle is invalid");
	return handle_;
}

void QueryPool::host_reset(uint32_t first_query, uint32_t query_count) const
{
	assert(device_.is_enabled("VK_EXT_host_query_reset") &&
	       "VK_EXT_host_query_reset needs to be enabled to call QueryPool::host_reset");
	device_.get_handle().resetQueryPool(handle_, first_query, query_count);
}

vk::Result QueryPool::get_results(uint32_t first_query, uint32_t num_queries, size_t result_bytes, void *results, vk::DeviceSize stride, vk::QueryResultFlags flags) const
{
	return device_.get_handle().getQueryPoolResults(handle_, first_query, num_queries, result_bytes, results, stride, flags);
}
}        // namespace xihe::backend
