#include "fx_test_pass.h"

namespace xihe::rendering
{
FxComputePass::FxComputePass(const std::string &name, RenderContext &render_context, RdgPassType pass_type) :
    RdgPass(name, render_context, pass_type),
    calculate_shader_{"particles/particle_calculate.comp"},
    integrate_shader_{"particles/particle_integrate.comp"}
{

	std::vector<glm::vec3> attractors = {
	    glm::vec3(5.0f, 0.0f, 0.0f),
	    glm::vec3(-5.0f, 0.0f, 0.0f),
	    glm::vec3(0.0f, 0.0f, 5.0f),
	    glm::vec3(0.0f, 0.0f, -5.0f),
	    glm::vec3(0.0f, 4.0f, 0.0f),
	    glm::vec3(0.0f, -8.0f, 0.0f),
	};


}

void FxComputePass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target, std::vector<backend::CommandBuffer *> secondary_command_buffers)
{
	RdgPass::execute(command_buffer, render_target, secondary_command_buffers);
}
}
