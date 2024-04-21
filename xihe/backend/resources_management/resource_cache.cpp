#include "resource_cache.h"

#include "backend/resources_management/resource_record.h"
#include "backend/resources_management/resource_caching.h"

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
}

ResourceCache::ResourceCache(Device &device) :
    device_(device)
{
}

void ResourceCache::set_pipeline_cache(vk::PipelineCache pipeline_cache)
{
	std::string entry_point{"main"};
	return reque
}

ShaderModule & ResourceCache::request_shader_module(vk::ShaderStageFlagBits stage, const ShaderSource &glsl_source, const ShaderVariant &shader_variant)
{}

PipelineLayout & ResourceCache::request_pipeline_layout(const std::vector<ShaderModule *> &shader_modules)
{}

DescriptorSetLayout & ResourceCache::request_descriptor_set_layout(const uint32_t set_index, const std::vector<ShaderModule *> &shader_modules, const std::vector<ShaderResource> &set_resources)
{}

GraphicsPipeline & ResourceCache::request_graphics_pipeline(PipelineState &pipeline_state)
{}

ComputePipeline & ResourceCache::request_compute_pipeline(PipelineState &pipeline_state)
{}

DescriptorSet & ResourceCache::request_descriptor_set(DescriptorSetLayout &descriptor_set_layout, const BindingMap<vk::DescriptorBufferInfo> &buffer_infos, const BindingMap<vk::DescriptorImageInfo> &image_infos)
{}

RenderPass & ResourceCache::request_render_pass(const std::vector<rendering::Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses)
{}

Framebuffer & ResourceCache::request_framebuffer(const rendering::RenderTarget &render_target, const RenderPass &render_pass)
{}

void ResourceCache::clear()
{
	
}
}        // namespace xihe::backend
