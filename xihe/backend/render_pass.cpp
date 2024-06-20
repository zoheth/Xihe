#include "render_pass.h"

#include "device.h"

namespace xihe::backend
{
namespace
{
inline const vk::AttachmentReference *get_depth_resolve_reference(const vk::SubpassDescription &subpass_description)
{
	/*auto description_depth_resolve = static_cast<const vk::SubpassDescriptionDepthStencilResolve *>(subpass_description.pNext);

	const vk::AttachmentReference *depth_resolve_attachment = nullptr;
	if (description_depth_resolve)
	{
		depth_resolve_attachment = description_depth_resolve->pDepthStencilResolveAttachment;
	}

	return depth_resolve_attachment;*/
	return nullptr;
}

std::vector<vk::AttachmentDescription> get_attachment_descriptions(const std::vector<rendering::Attachment> &attachments, const std::vector<common::LoadStoreInfo> &load_store_infos)
{
	std::vector<vk::AttachmentDescription> attachment_descriptions;

	for (size_t i = 0U; i < attachments.size(); ++i)
	{
		vk::AttachmentDescription attachment{};

		attachment.format        = attachments[i].format;
		attachment.samples       = attachments[i].samples;
		attachment.initialLayout = attachments[i].initial_layout;
		attachment.finalLayout =
		    common::is_depth_format(attachment.format) ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eColorAttachmentOptimal;

		if (i < load_store_infos.size())
		{
			attachment.loadOp         = load_store_infos[i].load_op;
			attachment.storeOp        = load_store_infos[i].store_op;
			attachment.stencilLoadOp  = load_store_infos[i].load_op;
			attachment.stencilStoreOp = load_store_infos[i].store_op;
		}

		attachment_descriptions.push_back(attachment);
	}

	return attachment_descriptions;
}

std::vector<vk::SubpassDependency> get_subpass_dependencies(const size_t subpass_count)
{
	std::vector<vk::SubpassDependency> dependencies(subpass_count - 1);

	if (subpass_count > 1)
	{
		for (uint32_t i = 0; i < to_u32(dependencies.size()); ++i)
		{
			// Transition input attachments from color attachment to shader read
			dependencies[i].srcSubpass      = i;
			dependencies[i].dstSubpass      = i + 1;
			dependencies[i].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			dependencies[i].dstStageMask    = vk::PipelineStageFlagBits::eFragmentShader;
			dependencies[i].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite;
			dependencies[i].dstAccessMask   = vk::AccessFlagBits::eInputAttachmentRead;
			dependencies[i].dependencyFlags = vk::DependencyFlagBits::eByRegion;
		}
	}

	return dependencies;
}

void set_attachment_layouts(std::vector<vk::SubpassDescription> &subpass_descriptions, std::vector<vk::AttachmentDescription> &attachment_descriptions)
{
	// Make the initial layout same as in the first subpass using that attachment
	for (auto &subpass : subpass_descriptions)
	{
		for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
		{
			auto &reference = subpass.pColorAttachments[k];
			// Set it only if not defined yet
			if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined)
			{
				attachment_descriptions[reference.attachment].initialLayout = reference.layout;
			}
		}

		for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k)
		{
			auto &reference = subpass.pInputAttachments[k];
			// Set it only if not defined yet
			if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined)
			{
				attachment_descriptions[reference.attachment].initialLayout = reference.layout;
			}
		}

		if (subpass.pDepthStencilAttachment)
		{
			auto &reference = *subpass.pDepthStencilAttachment;
			// Set it only if not defined yet
			if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined)
			{
				attachment_descriptions[reference.attachment].initialLayout = reference.layout;
			}
		}

		if (subpass.pResolveAttachments)
		{
			for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
			{
				auto &reference = subpass.pResolveAttachments[k];
				// Set it only if not defined yet
				if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined)
				{
					attachment_descriptions[reference.attachment].initialLayout = reference.layout;
				}
			}
		}

		if (const auto depth_resolve = get_depth_resolve_reference(subpass))
		{
			// Set it only if not defined yet
			if (attachment_descriptions[depth_resolve->attachment].initialLayout == vk::ImageLayout::eUndefined)
			{
				attachment_descriptions[depth_resolve->attachment].initialLayout = depth_resolve->layout;
			}
		}
	}

	// Make the final layout same as the last subpass layout
	{
		auto &subpass = subpass_descriptions.back();

		for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
		{
			const auto &reference = subpass.pColorAttachments[k];

			attachment_descriptions[reference.attachment].finalLayout = reference.layout;
		}

		for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k)
		{
			const auto &reference = subpass.pInputAttachments[k];

			attachment_descriptions[reference.attachment].finalLayout = reference.layout;

			// Do not use depth attachment if used as input
			if (common::is_depth_format(attachment_descriptions[reference.attachment].format))
			{
				subpass.pDepthStencilAttachment = nullptr;
			}
		}

		if (subpass.pDepthStencilAttachment)
		{
			const auto &reference = *subpass.pDepthStencilAttachment;

			attachment_descriptions[reference.attachment].finalLayout = reference.layout;
		}

		if (subpass.pResolveAttachments)
		{
			for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
			{
				const auto &reference = subpass.pResolveAttachments[k];

				attachment_descriptions[reference.attachment].finalLayout = reference.layout;
			}
		}

		if (const auto depth_resolve = get_depth_resolve_reference(subpass))
		{
			attachment_descriptions[depth_resolve->attachment].finalLayout = depth_resolve->layout;
		}
	}
}
}        // namespace

RenderPass::RenderPass(Device &device, const std::vector<rendering::Attachment> &attachments, const std::vector<common::LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses):
    VulkanResource{VK_NULL_HANDLE, &device},
    subpass_count_{std::max<size_t>(1, subpasses.size())}
{
	// assert(device.is_enabled(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME));
	create_renderpass(attachments, load_store_infos, subpasses);
}

RenderPass::RenderPass(RenderPass &&other) :
    VulkanResource{std::move(other)},
    subpass_count_{other.subpass_count_},
    color_output_count_{other.color_output_count_}
{}

RenderPass::~RenderPass()
{
	if (has_device())
	{
		get_device().get_handle().destroyRenderPass(get_handle());
	}
}

uint32_t RenderPass::get_color_output_count(uint32_t subpass_index) const
{
	return color_output_count_[subpass_index];
}

vk::Extent2D RenderPass::get_render_area_granularity() const
{
	vk::Extent2D granularity;

	get_device().get_handle().getRenderAreaGranularity(get_handle(), &granularity);

	return granularity;
}

void RenderPass::create_renderpass(const std::vector<rendering::Attachment> &attachments, const std::vector<common::LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses)
{
	auto attachment_descriptions = get_attachment_descriptions(attachments, load_store_infos);

	std::vector<std::vector<vk::AttachmentReference>> input_attachments{subpass_count_};
	std::vector<std::vector<vk::AttachmentReference>> color_attachments{subpass_count_};
	std::vector<std::vector<vk::AttachmentReference>> depth_stencil_attachments{subpass_count_};
	std::vector<std::vector<vk::AttachmentReference>> color_resolve_attachments{subpass_count_};
	std::vector<std::vector<vk::AttachmentReference>> depth_resolve_attachments{subpass_count_};

	std::string new_debug_name{};
	const bool  needs_debug_name = get_debug_name().empty();
	if (needs_debug_name)
	{
		new_debug_name = fmt::format("RP with {} subpasses:\n", subpasses.size());
	}

	for (size_t i = 0; i < subpasses.size(); ++i)
	{
		auto &subpass = subpasses[i];

		if (needs_debug_name)
		{
			new_debug_name += fmt::format("\t[{}]: {}\n", i, subpass.debug_name);
		}

		// Fill color attachments references
		for (auto o_attachment : subpass.output_attachments)
		{
			auto  initial_layout = attachments[o_attachment].initial_layout == vk::ImageLayout::eUndefined ? vk::ImageLayout::eColorAttachmentOptimal : attachments[o_attachment].initial_layout;
			auto &description    = attachment_descriptions[o_attachment];
			if (!common::is_depth_format(description.format))
			{
				color_attachments[i].emplace_back(o_attachment, initial_layout);
			}
		}

		// Fill input attachments references
		for (auto i_attachment : subpass.input_attachments)
		{
			auto default_layout = common::is_depth_format(attachment_descriptions[i_attachment].format) ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
			auto initial_layout = attachments[i_attachment].initial_layout == vk::ImageLayout::eUndefined ? default_layout : attachments[i_attachment].initial_layout;
			// todo 
			input_attachments[i].emplace_back(i_attachment, default_layout);
		}

		for (auto r_attachment : subpass.color_resolve_attachments)
		{
			auto initial_layout = attachments[r_attachment].initial_layout == vk::ImageLayout::eUndefined ? vk::ImageLayout::eColorAttachmentOptimal : attachments[r_attachment].initial_layout;
			color_resolve_attachments[i].emplace_back(r_attachment, initial_layout);
		}

		if (!subpass.disable_depth_stencil_attachment)
		{
			if (subpass.depth_stencil_attachment)
			{
				auto &attachment     = attachments.at(subpass.depth_stencil_attachment);
				auto  initial_layout = attachment.initial_layout == vk::ImageLayout::eUndefined ? vk::ImageLayout::eDepthStencilAttachmentOptimal : attachment.initial_layout;
				depth_stencil_attachments[i].emplace_back(subpass.depth_stencil_attachment, initial_layout); 
			}
			else
			{
				// Assumption: depth stencil attachment appears in the list before any depth stencil resolve attachment
				auto it = std::ranges::find_if(attachments, [](const rendering::Attachment attachment) { return common::is_depth_format(attachment.format); });
				if (it != attachments.end())
				{
					auto i_depth_stencil = to_u32(std::distance(attachments.begin(), it));
					auto initial_layout  = it->initial_layout == vk::ImageLayout::eUndefined ? vk::ImageLayout::eDepthStencilAttachmentOptimal : it->initial_layout;
					depth_stencil_attachments[i].emplace_back(i_depth_stencil, initial_layout);

					if (subpass.depth_stencil_resolve_mode != vk::ResolveModeFlagBits::eNone)
					{
						auto i_depth_stencil_resolve = subpass.depth_stencil_resolve_attachment;
						initial_layout               = attachments[i_depth_stencil_resolve].initial_layout == vk::ImageLayout::eUndefined ? vk::ImageLayout::eDepthStencilAttachmentOptimal : attachments[i_depth_stencil_resolve].initial_layout;
						depth_resolve_attachments[i].emplace_back(i_depth_stencil_resolve, initial_layout);
					}
				}	
			}
		}
	}

	std::vector<vk::SubpassDescription> subpass_descriptions;
	subpass_descriptions.reserve(subpass_count_);
	vk::SubpassDescriptionDepthStencilResolve depth_resolve{};
	for (size_t i = 0; i < subpasses.size(); ++i)
	{
		auto &subpass = subpasses[i];

		vk::SubpassDescription subpass_description{
			{},
			vk::PipelineBindPoint::eGraphics,
		    input_attachments[i],
		    color_attachments[i],
		    color_resolve_attachments[i]
		};

		/*subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

		subpass_description.pInputAttachments    = input_attachments[i].empty() ? nullptr : input_attachments[i].data();
		subpass_description.inputAttachmentCount = to_u32(input_attachments[i].size());

		subpass_description.pColorAttachments    = color_attachments[i].empty() ? nullptr : color_attachments[i].data();
		subpass_description.colorAttachmentCount = to_u32(color_attachments[i].size());

		subpass_description.pResolveAttachments = color_resolve_attachments[i].empty() ? nullptr : color_resolve_attachments[i].data();*/

		subpass_description.pDepthStencilAttachment = nullptr;
		if (!depth_stencil_attachments[i].empty())
		{
			subpass_description.pDepthStencilAttachment = depth_stencil_attachments[i].data();

			if (!depth_resolve_attachments[i].empty())
			{
				// If the pNext list of VkSubpassDescription includes a VkSubpassDescriptionDepthStencilResolve structure,
				// then that structure describes multisample resolve operations for the depth/stencil attachment in a subpass
				depth_resolve.depthResolveMode = subpass.depth_stencil_resolve_mode;

				// VkSubpassDescription cannot have pNext point to a VkSubpassDescriptionDepthStencilResolveKHR containing a VkAttachmentReference
				// depth_resolve.pDepthStencilResolveAttachment = depth_resolve_attachments[i].data();
				// subpass_description.pNext                    = &depth_resolve;

				auto &reference = depth_resolve_attachments[i][0];
				// Set it only if not defined yet
				if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined)
				{
					attachment_descriptions[reference.attachment].initialLayout = reference.layout;
				}
			}
		}

		subpass_descriptions.push_back(subpass_description);
	}

	// Default subpass
	if (subpasses.empty())
	{
		vk::SubpassDescription subpass_description{};
		subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		uint32_t default_depth_stencil_attachment{vk::AttachmentUnused};

		for (uint32_t k = 0U; k < to_u32(attachment_descriptions.size()); ++k)
		{
			if (common::is_depth_format(attachments[k].format))
			{
				if (default_depth_stencil_attachment == vk::AttachmentUnused)
				{
					default_depth_stencil_attachment = k;
				}
				continue;
			}

			color_attachments[0].emplace_back(k, vk::ImageLayout::eGeneral);
		}

		subpass_description.pColorAttachments = color_attachments[0].data();

		if (default_depth_stencil_attachment != VK_ATTACHMENT_UNUSED)
		{
			depth_stencil_attachments[0].emplace_back(default_depth_stencil_attachment, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			subpass_description.pDepthStencilAttachment = depth_stencil_attachments[0].data();
		}

		subpass_descriptions.push_back(subpass_description);
	}

	set_attachment_layouts(subpass_descriptions, attachment_descriptions);

	color_output_count_.reserve(subpass_count_);
	for (size_t i = 0; i < subpass_count_; i++)
	{
		color_output_count_.push_back(to_u32(color_attachments[i].size()));
	}

	const auto &subpass_dependencies = get_subpass_dependencies(subpass_count_);

	vk::RenderPassCreateInfo create_info{};
	create_info.attachmentCount = to_u32(attachment_descriptions.size());
	create_info.pAttachments    = attachment_descriptions.data();
	create_info.subpassCount    = to_u32(subpass_descriptions.size());
	create_info.pSubpasses      = subpass_descriptions.data();
	create_info.dependencyCount = to_u32(subpass_dependencies.size());
	create_info.pDependencies   = subpass_dependencies.data();

	auto result = get_device().get_handle().createRenderPass(&create_info, nullptr, &get_handle());

	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Cannot create RenderPass"};
	}

	if (needs_debug_name)
	{
		set_debug_name(new_debug_name);
	}
}
}        // namespace xihe::backend
