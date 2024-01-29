#pragma once

#include "common/error.h"

XH_DISABLE_WARNINGS()
#include <spirv_glsl.hpp>
XH_ENABLE_WARNINGS()

#include "backend/shader_module.h"

namespace xihe::backend
{
class SpirvReflection
{
  public:
	static bool reflect_shader_resources(vk::ShaderStageFlagBits      stage,
	                                     const std::vector<uint32_t> &spirv,
	                                     std::vector<ShaderResource> &resources,
	                                     const ShaderVariant         &variant);

  private:
	static void parse_shader_resources(const spirv_cross::Compiler &compiler,
	                                   vk::ShaderStageFlagBits      stage,
	                                   std::vector<ShaderResource> &resources,
	                                   const ShaderVariant         &variant);

	static void parse_push_constants(const spirv_cross::Compiler &compiler,
	                                 vk::ShaderStageFlagBits      stage,
	                                 std::vector<ShaderResource> &resources,
	                                 const ShaderVariant         &variant);

	static void parse_specialization_constants(const spirv_cross::Compiler &compiler,
	                                           vk::ShaderStageFlagBits      stage,
	                                           std::vector<ShaderResource> &resources,
	                                           const ShaderVariant         &variant);
};
}        // namespace xihe::backend