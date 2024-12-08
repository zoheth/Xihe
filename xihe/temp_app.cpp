#include "temp_app.h"

#include "backend/shader_compiler/glsl_compiler.h"
#include "rendering/passes/geometry_pass.h"
#include "rendering/passes/lighting_pass.h"
#include "rendering/passes/bloom_pass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/light.h"
#include "scene_graph/components/mesh.h"

namespace xihe
{
TempApp::TempApp()
{
	// Adding device extensions
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	backend::GlslCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
}

bool TempApp::prepare(Window *window)
{
	if (!XiheApp::prepare(window))
	{
		return false;
	}

	load_scene("scenes/sponza/Sponza01.gltf");
	// load_scene("scenes/cube.gltf");
	assert(scene_ && "Scene not loaded");

	update_bindless_descriptor_sets();

	auto &camera_node = xihe::sg::add_free_camera(*scene_, "main_camera", render_context_->get_surface_extent());
	auto  camera      = &camera_node.get_component<xihe::sg::Camera>();

	// geometry pass
	{
		auto geometry_pass = std::make_unique<rendering::GeometryPass>(scene_->get_components<sg::Mesh>(), *camera);

		graph_builder_->add_pass("Geometry", std::move(geometry_pass))

		    .attachments({{rendering::AttachmentType::kDepth, "depth"},
		                  {rendering::AttachmentType::kColor, "albedo"},
		                  {rendering::AttachmentType::kColor, "normal", vk::Format::eA2B10G10R10UnormPack32}})

		    .shader({"deferred/geometry.vert", "deferred/geometry.frag"})

		    .finalize();
	}

	// lighting pass
	{
		auto lighting_pass = std::make_unique<rendering::LightingPass>(scene_->get_components<sg::Light>(), *camera);

		graph_builder_->add_pass("Lighting", std::move(lighting_pass))

		    .bindables({{rendering::BindableType::kSampled, "depth"},
		                {rendering::BindableType::kSampled, "albedo"},
		                {rendering::BindableType::kSampled, "normal"}})

			.attachments({{rendering::AttachmentType::kColor, "lighting", vk::Format::eR16G16B16A16Sfloat}})

		    .shader({"deferred/lighting.vert", "deferred/lighting_simple.frag"})

			.finalize();
	}

	// bloom pass
	{
		auto extract_pass = std::make_unique<rendering::BloomExtractPass>();

		graph_builder_->add_pass("Bloom Extract", std::move(extract_pass))
		    .bindables({{rendering::BindableType::kSampled, "lighting"},
		                {rendering::BindableType::kStorageWrite, "bloom_extract", vk::Format::eR16G16B16A16Sfloat, vk::Extent3D{640,360,1}}})
		    .shader({"post_processing/bloom_extract.comp"})
		    .finalize();

		auto downsample_pass0 = std::make_unique<rendering::BloomDownsamplePass>(1.0f);
		graph_builder_->add_pass("Bloom Downsample 0", std::move(downsample_pass0))
		    .bindables({{rendering::BindableType::kSampled, "bloom_extract"},
		                {rendering::BindableType::kStorageWrite, "bloom_down_sample_0", vk::Format::eR16G16B16A16Sfloat, vk::Extent3D{320, 180, 1}}})
		    .shader({"post_processing/bloom_downsample.comp"})
		    .finalize();

		auto downsample_pass1 = std::make_unique<rendering::BloomDownsamplePass>(0.9f);
		graph_builder_->add_pass("Bloom Downsample 1", std::move(downsample_pass1))
		    .bindables({{rendering::BindableType::kSampled, "bloom_down_sample_0"},
		                {rendering::BindableType::kStorageWrite, "bloom_down_sample_1", vk::Format::eR16G16B16A16Sfloat, vk::Extent3D{160, 90, 1}}})
		    .shader({"post_processing/bloom_downsample.comp"})
		    .finalize();

		auto downsample_pass2 = std::make_unique<rendering::BloomDownsamplePass>(0.7f);
		graph_builder_->add_pass("Bloom Downsample 2", std::move(downsample_pass2))
		    .bindables({{rendering::BindableType::kSampled, "bloom_down_sample_1"},
		                {rendering::BindableType::kStorageWrite, "bloom_down_sample_2", vk::Format::eR16G16B16A16Sfloat, vk::Extent3D{80, 45, 1}}})
		    .shader({"post_processing/bloom_downsample.comp"})
		    .finalize();

		auto upsample_pass0 = std::make_unique<rendering::BloomComputePass>();
		graph_builder_->add_pass("Bloom Upsample 0", std::move(upsample_pass0))
		    .bindables({{rendering::BindableType::kSampled, "bloom_down_sample_2"},
		                {rendering::BindableType::kStorageWrite, "bloom_up_sample_0", vk::Format::eR16G16B16A16Sfloat, vk::Extent3D{160, 90, 1}}})
		    .shader({"post_processing/bloom_upsample.comp"})
		    .finalize();

		auto upsample_pass1 = std::make_unique<rendering::BloomComputePass>();
		graph_builder_->add_pass("Bloom Upsample 1", std::move(upsample_pass1))
		    .bindables({{rendering::BindableType::kSampled, "bloom_up_sample_0"},
		                {rendering::BindableType::kStorageWrite, "bloom_up_sample_1", vk::Format::eR16G16B16A16Sfloat, vk::Extent3D{320, 180, 1}}})
		    .shader({"post_processing/bloom_upsample.comp"})
		    .finalize();

		auto upsample_pass2 = std::make_unique<rendering::BloomComputePass>();
		graph_builder_->add_pass("Bloom Upsample 2", std::move(upsample_pass2))
		    .bindables({{rendering::BindableType::kSampled, "bloom_up_sample_1"},
		                {rendering::BindableType::kStorageWrite, "bloom_up_sample_2", vk::Format::eR16G16B16A16Sfloat, vk::Extent3D{640, 360, 1}}})
		    .shader({"post_processing/bloom_upsample.comp"})
		    .finalize();
	}

	// composite pass
	{
		auto composite_pass = std::make_unique<rendering::BloomCompositePass>();
		graph_builder_->add_pass("Bloom Composite", std::move(composite_pass))
		    .bindables({{rendering::BindableType::kSampled, "lighting"},
		                {rendering::BindableType::kSampled, "bloom_up_sample_2"}})
		    .shader({"post_processing/bloom_composite.vert", "post_processing/bloom_composite.frag"})
		    .present()
		    .finalize();
	}
	{
		
	}

	graph_builder_->build();

	return true;
}

void TempApp::update(float delta_time)
{
	XiheApp::update(delta_time);
}

void TempApp::request_gpu_features(backend::PhysicalDevice &gpu)
{
	XiheApp::request_gpu_features(gpu);

	REQUEST_REQUIRED_FEATURE(gpu, vk::PhysicalDeviceDynamicRenderingFeatures, dynamicRendering);
}
}        // namespace xihe

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::TempApp>();
}