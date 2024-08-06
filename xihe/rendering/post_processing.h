#pragma once

#include "backend/command_buffer.h"
#include "rdg_pass.h"

namespace xihe::rendering
{
std::unique_ptr<backend::Sampler> get_linear_sampler(backend::Device &device);

void render_blur(backend::CommandBuffer &command_buffer, ComputeRdgPass &rdg_pass);
}        // namespace xihe::rendering
