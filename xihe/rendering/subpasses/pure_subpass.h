#pragma once
#include "rendering/subpass.h"

namespace xihe::rendering
{
class TestSubpass : public rendering::Subpass
{
  public:
	TestSubpass(rendering::RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader);

	void prepare() override;

	void draw(backend::CommandBuffer &command_buffer) override;

  private:
	struct Vertex
	{
		glm::vec3 position;
		Vertex(glm::vec3 position) :
		    position(position)
		{}
	};

	std::unique_ptr<backend::ShaderSource> vertex_shader_;

	std::unique_ptr<backend::ShaderSource> fragment_shader_;

	VertexInputState                 vertex_input_state_{};
	std::unique_ptr<backend::Buffer> vertex_buffer_{nullptr};
};
}