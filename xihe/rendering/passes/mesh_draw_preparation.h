#pragma once

#include "render_pass.h"
#include "gpu_scene.h"

namespace xihe::rendering
{
class MeshDrawPreparationPass : public RenderPass
{
public:
	MeshDrawPreparationPass(GpuScene &gpu_scene);

	void execute(backend::CommandBuffer &command_buffer, RenderFrame &active_frame, std::vector<ShaderBindable> input_bindables) override;

private:
	GpuScene &gpu_scene_;
};
}
