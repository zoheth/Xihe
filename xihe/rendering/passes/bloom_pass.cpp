#include "bloom_pass.h"

namespace xihe::rendering
{
namespace
{
vk::SamplerCreateInfo get_linear_sampler()
{
	auto sampler_info         = vk::SamplerCreateInfo{};
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	return sampler_info;
}
}        // namespace

void BloomExtractPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&comp_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto &src = input_bindables[0].image_view();
	auto &dst = input_bindables[1].image_view();

	const auto extent = dst.get_image().get_extent();

	ExtractPush push;

	command_buffer.push_constants(push);
	command_buffer.bind_image(src, 0, 0, 0);
	command_buffer.bind_image(dst, 0, 1, 0);
	command_buffer.dispatch((extent.width + 15) / 16, (extent.height + 15) / 16, 1);
}

BloomBlurPass::BloomBlurPass(bool horizontal) :
    horizontal_(horizontal)
{}

void BloomBlurPass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto                                &resource_cache     = command_buffer.get_device().get_resource_cache();
	auto                                &comp_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eCompute, get_compute_shader());
	std::vector<backend::ShaderModule *> shader_modules     = {&comp_shader_module};
	auto                                &pipeline_layout    = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto &src = input_bindables[0].image_view();
	auto &dst = input_bindables[1].image_view();

	const auto extent = dst.get_image().get_extent();

	BlurPush push;

	command_buffer.push_constants(push);
	command_buffer.bind_image(src, 0, 0, 0);
	command_buffer.bind_image(dst, 0, 1, 0);

	if (horizontal_)
	{
		command_buffer.dispatch((extent.width + 255) / 256, extent.height, 1);
	}
	else
	{
		command_buffer.dispatch(extent.width, (extent.height + 255) / 256, 1);
	}
}

void BloomCompositePass::execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables)
{
	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader());

	std::vector<backend::ShaderModule *> shader_modules = {&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	auto &original = input_bindables[0].image_view();
	auto &blurred  = input_bindables[1].image_view();

	CompositePush push;

	command_buffer.push_constants(push);

	command_buffer.bind_image(original, resource_cache.request_sampler(get_linear_sampler()), 0, 0, 0);
	command_buffer.bind_image(blurred, resource_cache.request_sampler(get_linear_sampler()), 0, 1, 0);

	RasterizationState rasterization_state;
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	command_buffer.draw(3, 1, 0, 0);
}
}        // namespace xihe::rendering
