#include "fx_test_pass.h"

namespace xihe::rendering
{
FxComputePass::FxComputePass(const std::string &name, RdgPassType pass_type, RenderContext &render_context) :
    RdgPass(name, pass_type, render_context)
{
	calculate_shader_ = backend::ShaderSource{"particles/particle_calculate.comp"};
	integrate_shader_ = backend::ShaderSource{"particles/particle_integrate.comp"};
}

void FxComputePass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers)
{
	RdgPass::execute(command_buffer, render_target, secondary_command_buffers);
}
}
