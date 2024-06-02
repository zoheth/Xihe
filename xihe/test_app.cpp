#include "test_app.h"

#include "backend/shader_module.h"
#include "platform/filesystem.h"
#include "rendering/subpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/camera.h"

xihe::TestSubpass::TestSubpass(rendering::RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader) :
    rendering::Subpass{render_context, (std::move(vertex_shader)), (std::move(fragment_shader))}
{}

void xihe::TestSubpass::prepare()
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

void xihe::TestSubpass::draw(backend::CommandBuffer &command_buffer)
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

bool xihe::TestApp::prepare(Window *window)
{
	if (!XiheApp::prepare(window))
	{
		return false;
	}
	load_scene("scenes/sponza/Sponza01.gltf");
	assert(scene_ && "Scene not loaded");
	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", render_context_->get_surface_extent());
	camera_           = &camera_node.get_component<xihe::sg::Camera>();

	backend::ShaderSource vert_shader("base.vert");
	backend::ShaderSource frag_shader("base.frag");

	auto scene_subpass = std::make_unique<rendering::ForwardSubpass>(
	    *render_context_,
	    std::move(vert_shader), std::move(frag_shader), *scene_, *camera_);


	/*auto vertex_shader   = backend::ShaderSource{"tests/test.vert"};
	auto fragment_shader = backend::ShaderSource{"tests/test.frag"};

	auto test_subpass = std::make_unique<TestSubpass>(*render_context_, std::move(vertex_shader), std::move(fragment_shader));*/

	std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
	// subpasses.push_back(std::move(test_subpass));
	subpasses.push_back(std::move(scene_subpass));

	render_pipeline_ = std::make_unique<rendering::RenderPipeline>(std::move(subpasses));

	return true;
}

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::TestApp>();
}
