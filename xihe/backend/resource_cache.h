#pragma once
#include <unordered_map>

#include "backend/shader_module.h"
#include "backend/pipeline_layout.h"
#include "backend/descriptor_set_layout.h"
#include "backend/descriptor_pool.h"
#include "backend/render_pass.h"

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
	std::unordered_map<std::size_t, PipelineLayout> pipeline_layouts;
	std::unordered_map<std::size_t, DescriptorSetLayout> descriptor_set_layouts;
	std::unordered_map<std::size_t, DescriptorPool> descriptor_pools;
	std::unordered_map<std::size_t, RenderPass> render_passes;
	std::unordered_map<std::size_t, 

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
