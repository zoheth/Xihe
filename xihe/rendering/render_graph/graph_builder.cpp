#include "graph_builder.h"

namespace xihe::rendering
{
PassBuilder &PassBuilder::shader(std::initializer_list<std::string> file_names)
{
	render_pass_.set_shader(file_names);
	return *this;
}

void PassBuilder::finish()
{
	graph_builder_.add_pass(pass_name_, std::move(pass_info_),
	                        std::move(render_pass_));
}

void GraphBuilder::add_pass(const std::string &name, PassInfo &&pass_info, RenderPass &&render_pass)
{
	is_dirty_ = true;

	std::vector<backend::Image> images;

	bool use_swapchain_image = false;
	for (const auto &output : pass_info.outputs)
	{
		if (output.type == RenderResourceType::kSwapchain)
		{
			use_swapchain_image = true;
		}
	}
	if (use_swapchain_image)
	{
		RenderTarget::CreateFunc create_func = [this, &pass_info](backend::Image &&swapchain_image) {
			auto                       &device = swapchain_image.get_device();
			std::vector<backend::Image> images;
			vk::Extent3D                swapchain_image_extent = swapchain_image.get_extent();
			if (auto swapchain_count = std::ranges::count_if(pass_info.outputs, [](const auto &output) {
				    return output.type == RenderResourceType::kSwapchain;
			    });
			    swapchain_count > 1)
			{
				throw std::runtime_error("Multiple swapchain outputs are not supported.");
			}
			for (const auto &output : pass_info.outputs)
			{
				if (output.type == RenderResourceType::kSwapchain)
				{
					images.push_back(std::move(swapchain_image));
					continue;
				}
				vk::Extent3D extent = swapchain_image_extent;
				
				backend::ImageBuilder image_builder{extent};
				image_builder.with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
				    .with_format(output.format);
				if (common::is_depth_format(output.format))
				{
					image_builder.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | output.usage);
				}
				else
				{
					image_builder.with_usage(vk::ImageUsageFlagBits::eColorAttachment | output.usage);
				}
				backend::Image image = image_builder.build(device);
				images.push_back(std::move(image));
			}
			return std::make_unique<RenderTarget>(std::move(images));
		};

		render_context_.set_render_target_function(create_func);
	}
	else
	{
		// todo	
	}
	PassNode pass_node{name, PassType::kRaster, std::move(pass_info)};

}
}        // namespace xihe::rendering
