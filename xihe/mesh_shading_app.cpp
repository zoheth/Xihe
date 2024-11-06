#include "mesh_shading_app.h"

#include "backend/shader_compiler/glsl_compiler.h"
#include "rendering/subpasses/meshlet_subpass.h"
#include "scene_graph/scripts/free_camera.h"
#include "scene_graph/scene.h"

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

	load_scene("scenes/sponza/Sponza01.gltf");
	assert(scene_ && "Scene not loaded");

	update_bindless_descriptor_sets();

	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", get_render_context().get_surface_extent());
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();

	auto subpass = std::make_unique<rendering::MeshletSubpass>(*render_context_, std::nullopt, backend::ShaderSource{"deferred/geometry_mesh.mesh"}, backend::ShaderSource{"deferred/geometry_mesh.frag"}, *scene_, *camera);

	std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
	subpasses.push_back(std::move(subpass));

	rendering::PassInfo pass_info{};
	pass_info.outputs = {
	    {rendering::RdgResourceType::kSwapchain, "swapchain"},
	    {rendering::RdgResourceType::kAttachment, "depth", common::get_suitable_depth_format(get_device()->get_gpu().get_handle())},
		{rendering::RdgResourceType::kAttachment, "normal", vk::Format::eA2B10G10R10UnormPack32},
	};

	rdg_builder_->add_raster_pass("mesh_shader_meshlet", std::move(pass_info), std::move(subpasses));

	return true;

}

void xihe::MeshShadingApp::update(float delta_time)
{
	XiheApp::update(delta_time);
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

//
//std::unique_ptr<xihe::Application> create_application()
//{
//	return std::make_unique<xihe::MeshShadingApp>();
//}