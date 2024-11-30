#pragma once
#include "resource_record.h"

#include <mutex>
#include <unordered_map>

#include "backend/descriptor_pool.h"
#include "backend/descriptor_set.h"
#include "backend/descriptor_set_layout.h"
#include "backend/pipeline.h"
#include "backend/pipeline_layout.h"
#include "backend/shader_module.h"
#include "common/vk_common.h"
#include "rendering/render_target.h"

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
	std::unordered_map<std::size_t, ShaderModule>        shader_modules;
	std::unordered_map<std::size_t, PipelineLayout>      pipeline_layouts;
	std::unordered_map<std::size_t, DescriptorSetLayout> descriptor_set_layouts;
	std::unordered_map<std::size_t, DescriptorPool>      descriptor_pools;
	std::unordered_map<std::size_t, GraphicsPipeline>    graphics_pipelines;
	std::unordered_map<std::size_t, ComputePipeline>     compute_pipelines;
	std::unordered_map<std::size_t, DescriptorSet>       descriptor_sets;
};

class ResourceCache
{
  public:
	ResourceCache(Device &device);

	ResourceCache(const ResourceCache &)            = delete;
	ResourceCache &operator=(const ResourceCache &) = delete;
	ResourceCache(ResourceCache &&)                 = delete;
	ResourceCache &operator=(ResourceCache &&)      = delete;

	void set_pipeline_cache(vk::PipelineCache pipeline_cache);

	ShaderModule &request_shader_module(vk::ShaderStageFlagBits stage, const ShaderSource &glsl_source, const ShaderVariant &shader_variant = {});

	PipelineLayout &request_pipeline_layout(const std::vector<ShaderModule *> &shader_modules, BindlessDescriptorSet *bindless_descriptor_set = nullptr);

	DescriptorSetLayout &request_descriptor_set_layout(const uint32_t                     set_index,
	                                                   const std::vector<ShaderModule *> &shader_modules,
	                                                   const std::vector<ShaderResource> &set_resources);

	GraphicsPipeline &request_graphics_pipeline(PipelineState &pipeline_state);
	ComputePipeline  &request_compute_pipeline(PipelineState &pipeline_state);

	DescriptorSet &request_descriptor_set(DescriptorSetLayout                        &descriptor_set_layout,
	                                      const BindingMap<vk::DescriptorBufferInfo> &buffer_infos,
	                                      const BindingMap<vk::DescriptorImageInfo>  &image_infos);

	BindlessDescriptorSet &request_bindless_descriptor_set()
	{
		if (!bindless_descriptor_set_)
		{
			bindless_descriptor_set_ = std::make_unique<BindlessDescriptorSet>(device_);
		}
		return *bindless_descriptor_set_;
	}

	void clear();
	void clear_pipelines();

  private:
	Device &device_;
	ResourceRecord recorder_{};

	std::unique_ptr<BindlessDescriptorSet> bindless_descriptor_set_;

	vk::PipelineCache pipeline_cache_{VK_NULL_HANDLE};

	ResourceCacheState state_;

	std::mutex descriptor_set_mutex_        = {};
	std::mutex pipeline_layout_mutex_       = {};
	std::mutex shader_module_mutex_         = {};
	std::mutex descriptor_set_layout_mutex_ = {};
	std::mutex graphics_pipeline_mutex_     = {};
	std::mutex compute_pipeline_mutex_      = {};
	std::mutex framebuffer_mutex_           = {};
};
}        // namespace backend
}        // namespace xihe
