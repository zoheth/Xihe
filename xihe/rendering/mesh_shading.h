#pragma once

#include "render_frame.h"

#include "render_context.h"

namespace xihe::rendering
{
class MeshShading
{
  public:
	void draw(backend::CommandBuffer &command_buffer);

  private:
	backend::PipelineLayout &prepare_pipeline_layout(backend::CommandBuffer &command_buffer, const std::vector<backend::ShaderModule *> &shader_modules);

	template <typename T>
	void update_uniform_buffer(backend::CommandBuffer &command_buffer, const T &ubo, size_t thread_index)
	{
		auto &render_frame = render_context_.get_active_frame();
		auto  allocation   = render_frame.allocate_buffer(vk::BufferUsageFlagBits::eUniformBuffer, sizeof(T), thread_index);

		allocation.update(ubo);

		command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
	}

	RenderContext &render_context_;

	backend::ShaderSource task_shader_;
	backend::ShaderSource mesh_shader_;
	backend::ShaderSource fragment_shader_;

	int32_t density_level_ = 2;
};
}        // namespace xihe::rendering
