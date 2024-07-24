#pragma once
#include "rendering/subpass.h"

namespace xihe::rendering
{
class CompositeSubpass : public Subpass
{
  public:
	CompositeSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader);

	void draw(backend::CommandBuffer &command_buffer) override;
	void prepare() override;

  private:
	std::unique_ptr<backend::Sampler> sampler_{nullptr};

	backend::PipelineLayout *pipeline_layout_{nullptr};
};
}        // namespace xihe::rendering
