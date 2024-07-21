#pragma once
#include "rendering/subpass.h"

namespace xihe::rendering
{
class CompositeSubpass : public Subpass
{
  public:
	CompositeSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader);
	void set_texture(const backend::ImageView *hdr_view, const backend::ImageView *bloom_view, const backend::Sampler *sampler);

	void draw(backend::CommandBuffer &command_buffer) override;
	void prepare() override;

  private:
	const backend::ImageView      *hdr_view_{nullptr};
	const backend::ImageView      *bloom_view_{nullptr};
	const backend::Sampler        *sampler_{nullptr};

	backend::PipelineLayout *pipeline_layout_{nullptr};
};
}        // namespace xihe::rendering
