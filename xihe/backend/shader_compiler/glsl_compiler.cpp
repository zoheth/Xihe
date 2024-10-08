#include "glsl_compiler.h"

#include "glslang/Include/glslang_c_shader_types.h"
#include "glslang/Public/ResourceLimits.h"
#include "SPIRV/GlslangToSpv.h"
#include "StandAlone/DirStackFileIncluder.h"

namespace xihe::backend
{
namespace
{
EShLanguage find_shader_language(vk::ShaderStageFlagBits stage)
{
	switch (stage)
	{
		case vk::ShaderStageFlagBits::eVertex:
			return EShLangVertex;
		case vk::ShaderStageFlagBits::eTessellationControl:
			return EShLangTessControl;
		case vk::ShaderStageFlagBits::eTessellationEvaluation:
			return EShLangTessEvaluation;
		case vk::ShaderStageFlagBits::eGeometry:
			return EShLangGeometry;
		case vk::ShaderStageFlagBits::eFragment:
			return EShLangFragment;
		case vk::ShaderStageFlagBits::eCompute:
			return EShLangCompute;
		case vk::ShaderStageFlagBits::eRaygenNV:
			return EShLangRayGenNV;
		case vk::ShaderStageFlagBits::eAnyHitNV:
			return EShLangAnyHitNV;
		case vk::ShaderStageFlagBits::eClosestHitNV:
			return EShLangClosestHitNV;
		case vk::ShaderStageFlagBits::eMissNV:
			return EShLangMissNV;
		case vk::ShaderStageFlagBits::eIntersectionNV:
			return EShLangIntersectNV;
		case vk::ShaderStageFlagBits::eCallableNV:
			return EShLangCallableNV;
		case vk::ShaderStageFlagBits::eTaskNV:
			return EShLangTaskNV;
		case vk::ShaderStageFlagBits::eMeshNV:
			return EShLangMeshNV;
		default:
			return EShLangCount;
	}
}
}        // namespace

glslang::EShTargetLanguage        GlslCompiler::env_target_language_         = glslang::EShTargetLanguage::EShTargetNone;
glslang::EShTargetLanguageVersion GlslCompiler::env_target_language_version_ = static_cast<glslang::EShTargetLanguageVersion>(0);

void GlslCompiler::set_target_environment(glslang::EShTargetLanguage target_language, glslang::EShTargetLanguageVersion target_language_version)
{
	env_target_language_         = target_language;
	env_target_language_version_ = target_language_version;
}

bool GlslCompiler::compile_to_spirv(vk::ShaderStageFlagBits stage, const std::vector<uint8_t> &glsl_source, const std::string &entry_point, const ShaderVariant &shader_variant, std::vector<std::uint32_t> &spirv, std::string &info_log)
{
	glslang::InitializeProcess();

	EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

	EShLanguage language = find_shader_language(stage);
	std::string source   = std::string(glsl_source.begin(), glsl_source.end());

	const char *file_name_list[1] = {""};
	const char *shader_source     = source.data();

	glslang::TShader shader(language);
	shader.setStringsWithLengthsAndNames(&shader_source, nullptr, file_name_list, 1);
	shader.setEntryPoint(entry_point.c_str());
	shader.setSourceEntryPoint(entry_point.c_str());
	shader.setPreamble(shader_variant.get_preamble().c_str());
	shader.addProcesses(shader_variant.get_processes());

	if (GlslCompiler::env_target_language_ != glslang::EShTargetNone)
	{
		shader.setEnvTarget(GlslCompiler::env_target_language_, GlslCompiler::env_target_language_version_);
	}

	DirStackFileIncluder include_dir;
	include_dir.pushExternalLocalDirectory("shaders");

	if (!shader.parse(GetDefaultResources(), 100, false, messages, include_dir))
	{
		info_log = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
		return false;
	}

	// Add shader to new program object.
	glslang::TProgram program;
	program.addShader(&shader);

	// Link program.
	if (!program.link(messages))
	{
		info_log = std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
		return false;
	}

	// Save any info log that was generated.
	if (shader.getInfoLog())
	{
		info_log += std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()) + "\n";
	}

	if (program.getInfoLog())
	{
		info_log += std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
	}

	glslang::TIntermediate *intermediate = program.getIntermediate(language);

	// Translate to SPIRV.
	if (!intermediate)
	{
		info_log += "Failed to get shared intermediate code.\n";
		return false;
	}

	spv::SpvBuildLogger logger;

	glslang::GlslangToSpv(*intermediate, spirv, &logger);

	info_log += logger.getAllMessages() + "\n";

	// Shutdown glslang library.
	glslang::FinalizeProcess();

	return true;
}
}        // namespace xihe::backend
