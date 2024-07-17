#include "shadow_pass.h"

#include "rendering/render_context.h"
#include "rendering/subpasses/shadow_subpass.h"
#include "scene_graph/scene.h"

namespace xihe::rendering
{
ShadowPass::ShadowPass(const std::string &name, RenderContext &render_context, sg::Scene &scene, sg::CascadeScript &cascade_script) :
    RdgPass{name, render_context}
{
	use_swapchain_image_ = false;

	std::vector<std::unique_ptr<rendering::Subpass>> subpasses{};

	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		auto subpass = std::make_unique<rendering::ShadowSubpass>(render_context, backend::ShaderSource{"shadow/csm.vert"}, backend::ShaderSource{"shadow/csm.frag"}, scene, cascade_script, i);
		subpass->set_depth_stencil_attachment(i);

		add_subpass(std::move(subpass));
	}

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

	load_store_  = std::vector<common::LoadStoreInfo>(kCascadeCount, {vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore});
	clear_value_ = std::vector<vk::ClearValue>(kCascadeCount, vk::ClearDepthStencilValue{0.0f, 0});

	// create_owned_render_target();
}

ShadowPass::~ShadowPass()
{
	render_context_.get_device().get_handle().destroySampler(shadowmap_sampler_);
}

std::unique_ptr<RenderTarget> ShadowPass::create_render_target(backend::Image &&swapchain_image) const
{
	auto &device = render_context_.get_device();

	backend::ImageBuilder shadow_image_builder{2048, 2048, 1};
	shadow_image_builder.with_format(common::get_suitable_depth_format(device.get_gpu().get_handle()));
	shadow_image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	shadow_image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

	std::vector<backend::Image> images;

	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		backend::Image shadow_map = shadow_image_builder.build(device);
		images.push_back(std::move(shadow_map));
	}

	return std::make_unique<RenderTarget>(std::move(images));
}

void ShadowPass::create_owned_render_target()
{
	auto &device = render_context_.get_device();

	backend::ImageBuilder shadow_image_builder{2048, 2048, 1};
	shadow_image_builder.with_format(common::get_suitable_depth_format(device.get_gpu().get_handle()));
	shadow_image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	shadow_image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY);

	std::vector<backend::Image> images;

	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		backend::Image shadow_map = shadow_image_builder.build(device);
		images.push_back(std::move(shadow_map));
	}

	render_target_ = std::make_unique<RenderTarget>(std::move(images));

	load_store_  = std::vector<common::LoadStoreInfo>(kCascadeCount, {vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare});
	clear_value_ = std::vector<vk::ClearValue>(kCascadeCount, vk::ClearDepthStencilValue{0.0f, 0});
}

std::vector<vk::DescriptorImageInfo> ShadowPass::get_descriptor_image_infos(RenderTarget &render_target) const
{
	auto &shadowmap_views = render_target.get_views();

	std::vector<vk::DescriptorImageInfo> descriptor_image_infos{};

	for (const auto &shadowmap : shadowmap_views)
	{
		vk::DescriptorImageInfo descriptor_image_info{};
		descriptor_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		descriptor_image_info.imageView   = shadowmap.get_handle();
		descriptor_image_info.sampler     = shadowmap_sampler_;

		descriptor_image_infos.push_back(descriptor_image_info);
	}
	return descriptor_image_infos;
}

vk::Sampler ShadowPass::get_shadowmap_sampler() const
{
	return shadowmap_sampler_;
}

void ShadowPass::begin_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target, vk::SubpassContents contents)
{
	auto &shadowmap_views = render_target.get_views();

	common::ImageMemoryBarrier memory_barrier{};

	memory_barrier.old_layout      = vk::ImageLayout::eUndefined;
	memory_barrier.new_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	memory_barrier.src_access_mask = vk::AccessFlagBits2::eNone;
	memory_barrier.dst_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
	memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eTopOfPipe;
	memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;


	for (const auto &shadowmap : shadowmap_views)
	{
		command_buffer.image_memory_barrier(shadowmap, memory_barrier);
	}

	RdgPass::begin_draw(command_buffer, render_target, contents);
}

void ShadowPass::end_draw(backend::CommandBuffer &command_buffer, RenderTarget &render_target)
{

	RdgPass::end_draw(command_buffer, render_target);

	auto &shadowmap_views = render_target.get_views();

	common::ImageMemoryBarrier memory_barrier{};

	memory_barrier.old_layout      = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	memory_barrier.new_layout      = vk::ImageLayout::eShaderReadOnlyOptimal;
	memory_barrier.src_access_mask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
	memory_barrier.dst_access_mask = vk::AccessFlagBits2::eShaderRead;
	memory_barrier.src_stage_mask  = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
	memory_barrier.dst_stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;

	for (auto &shadowmap : shadowmap_views)
	{
		command_buffer.image_memory_barrier(shadowmap, memory_barrier);
	}
}
}        // namespace xihe::rendering
