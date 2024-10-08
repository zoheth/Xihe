#pragma once

#include "common/error.h"

XH_DISABLE_WARNINGS()
#include <glslang/Public/ShaderLang.h>
XH_ENABLE_WARNINGS()

#include "backend/shader_module.h"

namespace xihe::backend
{
class GlslCompiler
{
  public:
	static void set_target_environment(glslang::EShTargetLanguage        target_language,
	                                   glslang::EShTargetLanguageVersion target_language_version);

	bool compile_to_spirv(vk::ShaderStageFlagBits     stage,
	                      const std::vector<uint8_t> &glsl_source,
	                      const std::string          &entry_point,
	                      const ShaderVariant        &shader_variant,
	                      std::vector<std::uint32_t> &spirv,
	                      std::string                &info_log);

  private:
	static glslang::EShTargetLanguage        env_target_language_;
	static glslang::EShTargetLanguageVersion env_target_language_version_;
};
}        // namespace xihe::backend
