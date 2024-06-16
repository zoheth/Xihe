#include "shadow_pass.h"

#include "rendering/render_context.h"
#include "rendering/subpasses/shadow_subpass.h"
#include "scene_graph/scene.h"

namespace xihe::rendering
{
ShadowPass::ShadowPass(RenderContext &render_context, sg::Scene &scene, sg::Camera &camera) :
    render_context_{render_context}
{
	use_swapchain_image_ = false;

	auto vertex_shader   = backend::ShaderSource{"shadow/csm.vert"};
	auto fragment_shader = backend::ShaderSource{"shadow/csm.frag"};

	auto shadow_subpass_0 = std::make_unique<rendering::ShadowSubpass>(render_context, std::move(backend::ShaderSource{"shadow/csm.vert"}), std::move(backend::ShaderSource{"shadow/csm.frag"}), scene, *dynamic_cast<sg::PerspectiveCamera *>(&camera), 0);
	shadow_subpass_0->set_depth_stencil_attachment(0);
	auto shadow_subpass_1 = std::make_unique<rendering::ShadowSubpass>(render_context, std::move(backend::ShaderSource{"shadow/csm.vert"}), std::move(backend::ShaderSource{"shadow/csm.frag"}), scene, *dynamic_cast<sg::PerspectiveCamera *>(&camera), 1);
	shadow_subpass_1->set_depth_stencil_attachment(1);
	auto shadow_subpass_2 = std::make_unique<rendering::ShadowSubpass>(render_context, std::move(backend::ShaderSource{"shadow/csm.vert"}), std::move(backend::ShaderSource{"shadow/csm.frag"}), scene, *dynamic_cast<sg::PerspectiveCamera *>(&camera), 2);
	shadow_subpass_2->set_depth_stencil_attachment(2);

	std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};
	subpasses.push_back(std::move(shadow_subpass_0));
	subpasses.push_back(std::move(shadow_subpass_1));
	subpasses.push_back(std::move(shadow_subpass_2));

	render_pipeline_ = std::make_unique<rendering::RenderPipeline>(std::move(subpasses));

	render_pipeline_->set_load_store({{vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare},
	                                  {vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare},
	                                  {vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare}});

	render_pipeline_->set_clear_value({vk::ClearDepthStencilValue{0.0f, 0},
	                                   vk::ClearDepthStencilValue{0.0f, 0},
	                                   vk::ClearDepthStencilValue{0.0f, 0}});

	vk::SamplerCreateInfo shadowmap_sampler_create_info{};
	shadowmap_sampler_create_info.minFilter     = vk::Filter::eLinear;
	shadowmap_sampler_create_info.magFilter     = vk::Filter::eLinear;
	shadowmap_sampler_create_info.addressModeU  = vk::SamplerAddressMode::eClampToBorder;
	shadowmap_sampler_create_info.addressModeV  = vk::SamplerAddressMode::eClampToBorder;
	shadowmap_sampler_create_info.addressModeW  = vk::SamplerAddressMode::eClampToBorder;
	shadowmap_sampler_create_info.borderColor   = vk::BorderColor::eFloatOpaqueWhite;
	shadowmap_sampler_create_info.compareEnable = VK_TRUE;
	shadowmap_sampler_create_info.compareOp     = vk::CompareOp::eGreaterOrEqual;

	shadowmap_sampler_ = render_context_.get_device().get_handle().createSampler(shadowmap_sampler_create_info);

	create_render_target();
}

ShadowPass::~ShadowPass()
{
	render_context_.get_device().get_handle().destroySampler(shadowmap_sampler_);
}

RenderTarget * ShadowPass::get_render_target() const
{
	return render_target_.get();
}

void ShadowPass::create_render_target()
{
	auto &device = render_context_.get_device();

	backend::ImageBuilder shadow_image_builder{2048, 2048, 1};
	shadow_image_builder.with_format(common::get_suitable_depth_format(device.get_gpu().get_handle()));
	shadow_image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	shadow_image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

	backend::Image shadow_map_0 = shadow_image_builder.build(device);
	backend::Image shadow_map_1 = shadow_image_builder.build(device);
	backend::Image shadow_map_2 = shadow_image_builder.build(device);

	std::vector<backend::Image> images;
	images.push_back(std::move(shadow_map_0));
	images.push_back(std::move(shadow_map_1));
	images.push_back(std::move(shadow_map_2));

	render_target_ =  std::make_unique<RenderTarget>(std::move(images));
}

void ShadowPass::execute(backend::CommandBuffer &command_buffer, RenderTarget &render_target) const
{
	begin_draw(command_buffer, render_target);
	render_pipeline_->draw(command_buffer, render_target);
	end_draw(command_buffer, render_target);
}

std::vector<vk::DescriptorImageInfo> ShadowPass::get_descriptor_image_infos(RenderTarget &render_target) const
{
	auto &shadowmap_views = render_target.get_views();

	std::vector<vk::DescriptorImageInfo> descriptor_image_infos{};

	for (const auto &shadowmap : shadowmap_views)
	{
		vk::DescriptorImageInfo descriptor_image_info{};
		descriptor_image_info.imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
		descriptor_image_info.imageView   = shadowmap.get_handle();
		descriptor_image_info.sampler     = shadowmap_sampler_;

		descriptor_image_infos.push_back(descriptor_image_info);
	}
	return descriptor_image_infos;
}

void ShadowPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	auto &shadowmap_views = render_target.get_views();

	common::ImageMemoryBarrier memory_barrier{};

	memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
	memory_barrier.new_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	memory_barrier.src_access_mask = vk::AccessFlagBits::eNone;
	memory_barrier.dst_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eTopOfPipe;
	memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;

	for (const auto &shadowmap : shadowmap_views)
	{
		command_buffer.image_memory_barrier(shadowmap, memory_barrier);
	}
}

void ShadowPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{
	auto &shadowmap_views = render_target.get_views();

	common::ImageMemoryBarrier memory_barrier{};

	memory_barrier.old_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	memory_barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
	memory_barrier.src_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	memory_barrier.dst_access_mask = vk::AccessFlagBits::eShaderRead;
	memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits::eFragmentShader;

	for (const auto &shadowmap : shadowmap_views)
	{
		command_buffer.image_memory_barrier(shadowmap, memory_barrier);
	}
}
}        // namespace xihe::rendering
