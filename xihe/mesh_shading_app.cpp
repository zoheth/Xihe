#include "mesh_shading_app.h"


#include "backend/shader_compiler/glsl_compiler.h"

xihe::MeshShadingApp::MeshShadingApp()
{
	// Adding device extensions
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	backend::GlslCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
}

bool xihe::MeshShadingApp::prepare(Window *window)
{
	if (!XiheApp::prepare(window))
	{
		return false;
	}


}

void xihe::MeshShadingApp::request_gpu_features(backend::PhysicalDevice &gpu)
{
	XiheApp::request_gpu_features(gpu);
	// Check whether the device supports task and mesh shaders
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, meshShader);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, meshShaderQueries);
	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceMeshShaderFeaturesEXT, taskShader);

	// Pipeline statistics
	auto &requested_features = gpu.get_mutable_requested_features();
	if (gpu.get_features().pipelineStatisticsQuery)
	{
		requested_features.pipelineStatisticsQuery = VK_TRUE;
	}
}
