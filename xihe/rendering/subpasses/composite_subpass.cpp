#include "composite_subpass.h"

#include "rendering/render_context.h"

namespace xihe::rendering
{
CompositeSubpass::CompositeSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader) :
    Subpass(render_context, std::move(vertex_shader), std::move(fragment_shader))
{
	auto sampler_info         = vk::SamplerCreateInfo{};
	sampler_info.magFilter    = vk::Filter::eLinear;
	sampler_info.minFilter    = vk::Filter::eLinear;
	sampler_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	sampler_ = std::make_unique<backend::Sampler>(render_context.get_device(), sampler_info);
}

void CompositeSubpass::draw(backend::CommandBuffer &command_buffer)
{
	auto &main_pass_views       = render_context_.get_active_frame().get_render_target("main_pass").get_views();
	auto &post_processing_views = render_context_.get_active_frame().get_render_target("blur_pass").get_views();

	command_buffer.bind_image(main_pass_views[0], *sampler_, 0, 0, 0);
	command_buffer.bind_image(post_processing_views[1], *sampler_, 0, 1, 0);
	command_buffer.bind_pipeline_layout(*pipeline_layout_);

	// A depth-stencil attachment exists in the default render pass, make sure we ignore it.
	DepthStencilState ds_state = {};
	ds_state.depth_test_enable = false;
	ds_state.depth_write_enable = false;
	ds_state.stencil_test_enable = false;
	ds_state.depth_compare_op = vk::CompareOp::eAlways;
	command_buffer.set_depth_stencil_state(ds_state);

	command_buffer.draw(3, 1, 0, 0);
}

void CompositeSubpass::prepare()
{
	auto &device = get_render_context().get_device();
	auto &vertex_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eVertex, get_vertex_shader());
	auto &fragment_module = device.get_resource_cache().request_shader_module(vk::ShaderStageFlagBits::eFragment, get_fragment_shader());
	pipeline_layout_      = &device.get_resource_cache().request_pipeline_layout({&vertex_module, &fragment_module});

}

}        // namespace xihe::rendering
