#include "render_target.h"

#include "backend/device.h"
#include "common/error.h"
#include "common/vk_common.h"

namespace xihe::rendering
{
const RenderTarget::CreateFunc RenderTarget::kDefaultCreateFunc = [](backend::Image &&swapchain_image) -> std::unique_ptr<RenderTarget> {
	vk::Format depth_format = common::get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle());

	backend::Image depth_image{swapchain_image.get_device(), swapchain_image.get_extent(),
	                           depth_format,
	                           vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
	                           VMA_MEMORY_USAGE_GPU_ONLY};

	std::vector<backend::Image> images;
	images.push_back(std::move(swapchain_image));
	images.push_back(std::move(depth_image));

	return std::make_unique<RenderTarget>(std::move(images));
};

RenderTarget::RenderTarget(std::vector<backend::Image> &&images, uint32_t base_layer, uint32_t layer_count) :
    device_{images.front().get_device()}, images_{std::move(images)}
{
	assert(!images_.empty() && "Should specify at least 1 image");

	// check that every image is 2D
	auto it = std::ranges::find_if(images_, [](backend::Image const &image) { return image.get_type() != vk::ImageType::e2D; });
	if (it != images_.end())
	{
		throw VulkanException{VK_ERROR_INITIALIZATION_FAILED, "Image type is not 2D"};
	}

	extent_.width  = images_.front().get_extent().width;
	extent_.height = images_.front().get_extent().height;

	// check that every image has the same extent
	it = std::find_if(std::next(images_.begin()),
	                  images_.end(),
	                  [this](backend::Image const &image) { return (extent_.width != image.get_extent().width) || (extent_.height != image.get_extent().height); });

	if (it != images_.end())
	{
	    throw VulkanException{VK_ERROR_INITIALIZATION_FAILED, "Extent size is not unique"};
	}

	for (auto &image : images_)
	{
		image_views_.emplace_back(image, vk::ImageViewType::e2D, vk::Format::eUndefined, 0, base_layer, 0, layer_count);
	}
}

RenderTarget::RenderTarget(std::vector<backend::ImageView> &&image_views) :
    device_{image_views.front().get_device()}
{
	assert(!image_views.empty() && "Should specify at least 1 image view");
	extent_.height = image_views.front().get_image().get_extent().height;
	extent_.width  = image_views.front().get_image().get_extent().width;

	for (auto &image_view : image_views)
	{
		images_.push_back(image_view.get_image());
	}
	image_views_ = std::move(image_views);
}

const vk::Extent2D &RenderTarget::get_extent() const
{
	return extent_;
}

std::vector<backend::ImageView> &RenderTarget::get_views()
{
	return image_views_;
}

void RenderTarget::set_first_bindless_descriptor_set_index(uint32_t index)
{
	first_bindless_descriptor_set_index_ = index;
}

uint32_t RenderTarget::get_first_bindless_descriptor_set_index() const
{
	return first_bindless_descriptor_set_index_;
}
}        // namespace xihe::rendering
