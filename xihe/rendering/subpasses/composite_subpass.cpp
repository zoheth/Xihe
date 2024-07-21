#include "composite_subpass.h"

#include "rendering/render_context.h"

namespace xihe::rendering
{
CompositeSubpass::CompositeSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader) :
    Subpass(render_context, std::move(vertex_shader), std::move(fragment_shader))
{}

void CompositeSubpass::set_texture(const backend::ImageView *hdr_view, const backend::ImageView *bloom_view, const backend::Sampler *sampler)
{
	hdr_view_   = hdr_view;
	bloom_view_ = bloom_view;
	sampler_    = sampler;
}

void CompositeSubpass::draw(backend::CommandBuffer &command_buffer)
{
	command_buffer.bind_image(*hdr_view_, *sampler_, 0, 0, 0);
	command_buffer.bind_image(*bloom_view_, *sampler_, 0, 1, 0);
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
