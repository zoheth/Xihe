#pragma once

namespace xihe::backend
{
class Device;
class ResourceCache
{
  public:
	ResourceCache(Device &device);

	ResourceCache(const ResourceCache &)            = delete;
	ResourceCache &operator=(const ResourceCache &) = delete;
	ResourceCache(ResourceCache &&)                 = delete;
	ResourceCache &operator=(ResourceCache &&)      = delete;

  private:
	Device &device_;
};
}        // namespace xihe::backend
