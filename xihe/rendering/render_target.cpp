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

RenderTarget::RenderTarget(std::vector<backend::Image> &&images) :
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
	// todo 确定是否需要这个检查
	/*if (it != images_.end())
	{
		throw VulkanException{VK_ERROR_INITIALIZATION_FAILED, "Extent size is not unique"};
	}*/

	for (auto &image : images_)
	{
		image_views_.emplace_back(image, vk::ImageViewType::e2D);
		attachments_.emplace_back(image.get_format(), image.get_sample_count(), image.get_usage());
	}
}

const vk::Extent2D &RenderTarget::get_extent() const
{
	return extent_;
}

const std::vector<backend::ImageView> & RenderTarget::get_views() const
{
	return image_views_;
}

const std::vector<Attachment> & RenderTarget::get_attachments() const
{
	return attachments_;
}

const std::vector<uint32_t> & RenderTarget::get_input_attachments() const
{
	return input_attachments_;
}

const std::vector<uint32_t> & RenderTarget::get_output_attachments() const
{
	return output_attachments_;
}

void RenderTarget::set_input_attachments(const std::vector<uint32_t> &input)
{
	input_attachments_ = input;
}

void RenderTarget::set_output_attachments(const std::vector<uint32_t> &output)
{
	output_attachments_ = output;
}

void RenderTarget::set_layout(uint32_t attachment, vk::ImageLayout layout)
{
	attachments_[attachment].initial_layout = layout;
}

vk::ImageLayout RenderTarget::get_layout(uint32_t attachment) const
{
	return attachments_[attachment].initial_layout;
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
