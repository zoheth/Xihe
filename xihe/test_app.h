#pragma once

#include "xihe_app.h"

namespace xihe
{

class TestApp : public XiheApp
{
  public:

	TestApp() = default;
	~TestApp() = default;

	bool prepare(Window *window) override;
	void update(float delta_time) override;
	void render(backend::CommandBuffer &command_buffer) override;

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
}        // namespace xihe
