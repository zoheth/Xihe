#pragma once
#include <unordered_map>

namespace xihe

{

namespace rendering
{
class RenderTarget;
}

namespace backend
{
class Device;

struct ResourceCacheState
{
	std::unordered_map<std::size_t, ShaderModule> shader_modules;
};

class ResourceCache
{
  public:
	ResourceCache(Device &device);

	ResourceCache(const ResourceCache &)            = delete;
	ResourceCache &operator=(const ResourceCache &) = delete;
	ResourceCache(ResourceCache &&)                 = delete;
	ResourceCache &operator=(ResourceCache &&)      = delete;

	void clear();

  private:
	Device &device_;


};
}        // namespace backend
}        // namespace xihe
