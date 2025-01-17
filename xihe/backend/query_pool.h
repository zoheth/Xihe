#pragma once

#include "common/vk_common.h"

namespace xihe::backend
{
class Device;

class QueryPool
{
  public:
	QueryPool(Device &device, vk::QueryPoolCreateInfo &info);

	QueryPool(const QueryPool &) = delete;

	QueryPool(QueryPool &&pool);

	~QueryPool();

	QueryPool &operator=(const QueryPool &) = delete;

	QueryPool &operator=(QueryPool &&) = delete;

	vk::QueryPool get_handle() const;

	void host_reset(uint32_t first_query, uint32_t query_count) const;

	vk::Result get_results(uint32_t first_query, uint32_t num_queries,
	                       size_t result_bytes, void *results, vk::DeviceSize stride, vk::QueryResultFlags flags) const;

  private:
	Device &device_;

	vk::QueryPool handle_{VK_NULL_HANDLE};
};
}        // namespace xihe::backend
