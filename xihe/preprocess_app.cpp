#include "preprocess_app.h"

#include "rendering/passes/geometry_pass.h"
#include "rendering/passes/skybox_pass.h"
#include "scene_graph/components/image.h"

namespace xihe
{
PreprocessApp::PreprocessApp()
{}

bool PreprocessApp::prepare(Window *window)
{
	using namespace rendering;

	if (!XiheApp::prepare(window))
	{
		return false;
	}

	load_scene("scenes/Box.gltf");
	// load_scene("scenes/cube.gltf");
	assert(scene_ && "Scene not loaded");

	asset_loader_ = std::make_unique<AssetLoader>(*device_);

	textures_.environment_cube = asset_loader_->load_texture_cube(*scene_, "env_cube", "textures/uffizi_rgba16f_cube.ktx");

	enum Target
	{
		IRRADIANCE     = 0,
		PREFILTEREDENV = 1
	};

	for (uint32_t target = 1; target < PREFILTEREDENV + 1; target++)
	{
		vk::Format format;
		int32_t    dim;

		switch (target)
		{
			case IRRADIANCE:
				format = vk::Format::eR32G32B32A32Sfloat;
				dim    = 64;
				break;
			case PREFILTEREDENV:
				format = vk::Format::eR16G16B16A16Sfloat;
				dim    = 512;
				break;
		}

		PrefilterPass::num_mips = static_cast<uint32_t>(floor(log2(dim))) + 1;

		backend::ImageBuilder image_builder(dim, dim);
		image_builder.with_format(format);
		image_builder.with_array_layers(6);
		image_builder.with_mip_levels(PrefilterPass::num_mips);
		image_builder.with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
		image_builder.with_flags(vk::ImageCreateFlagBits::eCubeCompatible);
		image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

		vk::SamplerCreateInfo sampler_info;
		sampler_info.magFilter     = vk::Filter::eLinear;
		sampler_info.minFilter     = vk::Filter::eLinear;
		sampler_info.mipmapMode    = vk::SamplerMipmapMode::eLinear;
		sampler_info.addressModeU  = vk::SamplerAddressMode::eClampToEdge;
		sampler_info.addressModeV  = vk::SamplerAddressMode::eClampToEdge;
		sampler_info.addressModeW  = vk::SamplerAddressMode::eClampToEdge;
		sampler_info.minLod        = 0.0f;
		sampler_info.maxLod        = static_cast<float>(PrefilterPass::num_mips);
		sampler_info.maxAnisotropy = 1.0f;
		sampler_info.borderColor   = vk::BorderColor::eFloatOpaqueWhite;

		textures_.prefiltered_cube.image      = image_builder.build_unique(*device_);
		textures_.prefiltered_cube.image_view = std::make_unique<backend::ImageView>(
		    *textures_.prefiltered_cube.image, vk::ImageViewType::eCube);
		textures_.prefiltered_cube.sampler = std::make_unique<backend::Sampler>(*device_, sampler_info);

		for (uint32_t m = 0; m < PrefilterPass::num_mips; m++)
		{
			for (uint32_t f = 0; f < 6; f++)

			{
				std::string suffix         = "_mip" + to_string(m) + "_face" + to_string(f);
				auto        prefilter_pass = std::make_unique<PrefilterPass>(*scene_->get_components<sg::Mesh>()[0], *textures_.environment_cube, m, f);

				PassAttachment attachment{AttachmentType::kColor, "prefilter_rt" + suffix};

				uint32_t mip_dim = dim * std::pow(0.5, m);

				attachment.extent_desc = ExtentDescriptor::Fixed({mip_dim, mip_dim, 1});
				attachment.format      = vk::Format::eR16G16B16A16Sfloat;
				attachment.is_external = true;

				auto          copy_dst_image_view = std::make_unique<backend::ImageView>(*textures_.prefiltered_cube.image, vk::ImageViewType::e2D, vk::Format::eUndefined, m, f, 1, 1);
				vk::ImageCopy copy_region{};
				copy_region.srcSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
				copy_region.srcSubresource.baseArrayLayer = 0;
				copy_region.srcSubresource.mipLevel       = 0;
				copy_region.srcSubresource.layerCount     = 1;
				copy_region.srcOffset                     = vk::Offset3D{0, 0, 0};

				copy_region.dstSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
				copy_region.dstSubresource.baseArrayLayer = f;
				copy_region.dstSubresource.mipLevel       = m;
				copy_region.dstSubresource.layerCount     = 1;
				copy_region.dstOffset                     = vk::Offset3D{0, 0, 0};

				copy_region.extent = vk::Extent3D{mip_dim, mip_dim, 1};

				graph_builder_->add_pass("prefilter_pass" + suffix, std::move(prefilter_pass))
				    .attachments({{attachment}})
				    .shader({"preprocess/filtercube.vert", "preprocess/prefilterenvmap.frag"})
				    .copy(0, std::move(copy_dst_image_view), copy_region)
				    .finalize();
			}
		}


	}
	graph_builder_->build();

	render_graph_->execute(false);

	get_device()->get_handle().waitIdle();

	graph_builder_.reset();
	render_graph_.reset();

	render_graph_  = std::make_unique<rendering::RenderGraph>(*render_context_);
	graph_builder_ = std::make_unique<rendering::GraphBuilder>(*render_graph_, *render_context_);

	auto &camera_node = sg::add_free_camera(*scene_, "default_camera", render_context_->get_surface_extent(), 3, 2);
	auto  camera      = &camera_node.get_component<sg::Camera>();

	auto skybox_pass = std::make_unique<SkyboxPass>(*scene_->get_components<sg::Mesh>()[0], textures_.prefiltered_cube, *camera);
	graph_builder_->add_pass("skybox_pass", std::move(skybox_pass))
	    .shader({"skybox.vert", "skybox.frag"})
	    .present()
	    .finalize();

	graph_builder_->build();
	return true;
}

void PreprocessApp::update(float delta_time)
{
	XiheApp::update(delta_time);
}

void PreprocessApp::request_gpu_features(backend::PhysicalDevice &gpu)
{
	XiheApp::request_gpu_features(gpu);
}

void PreprocessApp::draw_gui()
{
	XiheApp::draw_gui();
}

}        // namespace xihe

std::unique_ptr<xihe::Application> create_application()
{
	return std::make_unique<xihe::PreprocessApp>();
}
