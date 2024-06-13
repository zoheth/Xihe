#include "pure_subpass.h"

#include "rendering/render_context.h"

namespace xihe::rendering
{
TestSubpass::TestSubpass(rendering::RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader) :
    rendering::Subpass{render_context, (std::move(vertex_shader)), (std::move(fragment_shader))}
{}

void TestSubpass::prepare()
{
	auto &resource_cache = render_context_.get_device().get_resource_cache();

	vertex_shader_   = std::make_unique<backend::ShaderSource>("tests/test.vert");
	fragment_shader_ = std::make_unique<backend::ShaderSource>("tests/test.frag");

	resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, *vertex_shader_);
	resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, *fragment_shader_);

	vertex_input_state_.bindings.emplace_back(0, to_u32(sizeof(Vertex)), vk::VertexInputRate::eVertex);

	vertex_input_state_.attributes.emplace_back(0, 0, vk::Format::eR32G32B32Sfloat, to_u32(offsetof(Vertex, position)));

	backend::BufferBuilder buffer_builder{sizeof(Vertex) * 3};
	buffer_builder
	    .with_usage(vk::BufferUsageFlagBits::eVertexBuffer)
	    .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU);

	vertex_buffer_ = buffer_builder.build_unique(render_context_.get_device());

	std::vector<Vertex> vertices = {
	    {{0.0f, -0.5f, 0.0f}},
	    {{0.5f, 0.5f, 0.0f}},
	    {{-0.5f, 0.5f, 0.0f}},
	};

	vertex_buffer_->update(vertices.data(), vertices.size() * sizeof(Vertex));
}

void TestSubpass::draw(backend::CommandBuffer &command_buffer)
{
	auto &resource_cache = command_buffer.get_device().get_resource_cache();

	auto &vert_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eVertex, *vertex_shader_);
	auto &frag_shader_module = resource_cache.request_shader_module(vk::ShaderStageFlagBits::eFragment, *fragment_shader_);

	std::vector<backend::ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};

	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
	command_buffer.bind_pipeline_layout(pipeline_layout);

	RasterizationState rasterization_state{};
	rasterization_state.cull_mode = vk::CullModeFlagBits::eNone;
	command_buffer.set_rasterization_state(rasterization_state);

	command_buffer.set_vertex_input_state(vertex_input_state_);

	std::vector<std::reference_wrapper<const backend::Buffer>> buffers;
	buffers.emplace_back(std::ref(*vertex_buffer_));
	command_buffer.bind_vertex_buffers(0, buffers, {0});

	command_buffer.draw(3, 1, 0, 0);
}
}
