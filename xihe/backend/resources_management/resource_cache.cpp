#include "resource_cache.h"

#include "backend/resources_management/resource_caching.h"
#include "backend/resources_management/resource_record.h"

namespace xihe::backend
{

namespace
{
template <class T, class... A>
T &request_resource(Device &device, ResourceRecord &record, std::mutex &resource_mutex, std::unordered_map<std::size_t, T> &resources, A &...args)
{
	std::lock_guard<std::mutex> guard(resource_mutex);

	auto &res = request_resource(device, &record, resources, args...);

	return res;
}
}        // namespace

ResourceCache::ResourceCache(Device &device) :
    device_(device),
	bindless_descriptor_set_(device)
{
}

void ResourceCache::set_pipeline_cache(vk::PipelineCache pipeline_cache)
{
	pipeline_cache_ = pipeline_cache;
}

ShaderModule &ResourceCache::request_shader_module(vk::ShaderStageFlagBits stage, const ShaderSource &glsl_source, const ShaderVariant &shader_variant)
{
	std::string entry_point{"main"};
	return request_resource(device_, recorder_, shader_module_mutex_, state_.shader_modules, stage, glsl_source, entry_point, shader_variant);
}

PipelineLayout &ResourceCache::request_pipeline_layout(const std::vector<ShaderModule *> &shader_modules, BindlessDescriptorSet *bindless_descriptor_set)
{
	return request_resource(device_, recorder_, pipeline_layout_mutex_, state_.pipeline_layouts, shader_modules, bindless_descriptor_set);
}

DescriptorSetLayout &ResourceCache::request_descriptor_set_layout(const uint32_t set_index, const std::vector<ShaderModule *> &shader_modules, const std::vector<ShaderResource> &set_resources)
{
	return request_resource(device_, recorder_, descriptor_set_layout_mutex_, state_.descriptor_set_layouts, set_index, shader_modules, set_resources);
}

GraphicsPipeline &ResourceCache::request_graphics_pipeline(PipelineState &pipeline_state)
{
	return request_resource(device_, recorder_, graphics_pipeline_mutex_, state_.graphics_pipelines, pipeline_cache_, pipeline_state);
}


ComputePipeline &ResourceCache::request_compute_pipeline(PipelineState &pipeline_state)
{
	return request_resource(device_, recorder_, compute_pipeline_mutex_, state_.compute_pipelines, pipeline_cache_, pipeline_state);
}

DescriptorSet &ResourceCache::request_descriptor_set(DescriptorSetLayout &descriptor_set_layout, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos)
{
	auto &descriptor_pool = request_resource(device_, recorder_, descriptor_set_mutex_, state_.descriptor_pools, descriptor_set_layout);
	return request_resource(device_, recorder_, descriptor_set_mutex_, state_.descriptor_sets, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
}

void ResourceCache::clear()
{
	state_.shader_modules.clear();
	state_.pipeline_layouts.clear();
	state_.descriptor_sets.clear();
	state_.descriptor_set_layouts.clear();
	clear_pipelines();
}


void ResourceCache::clear_pipelines()
{
	state_.graphics_pipelines.clear();
	state_.compute_pipelines.clear();
}
}        // namespace xihe::backend
