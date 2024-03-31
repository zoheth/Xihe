#pragma once
#include <functional>
#include <memory>

#include "backend/image.h"
#include "backend/image_view.h"

namespace xihe
{
namespace backend
{
class Device;
}

namespace rendering
{

struct Attachment
{
	Attachment() = default;

	Attachment(vk::Format format, vk::SampleCountFlagBits samples, vk::ImageUsageFlags usage) :
	    format{format},
	    samples{samples},
	    usage{usage}
	{}

	vk::Format              format         = vk::Format::eUndefined;
	vk::SampleCountFlagBits samples        = vk::SampleCountFlagBits::e1;
	vk::ImageUsageFlags     usage          = vk::ImageUsageFlagBits::eSampled;
	vk::ImageLayout         initial_layout = vk::ImageLayout::eUndefined;
};

class RenderTarget
{
  public:
	using CreateFunc = std::function<std::unique_ptr<RenderTarget>(backend::Image &&)>;
	static const CreateFunc kDefaultCreateFunc;

	RenderTarget(std::vector<backend::Image> &&images);

	const vk::Extent2D                    &get_extent() const;
	const std::vector<backend::ImageView> &get_views() const;

	const std::vector<Attachment> &get_attachments() const;
	const std::vector<uint32_t>   &get_input_attachments() const;
	const std::vector<uint32_t>   &get_output_attachments() const;


  private:
	backend::Device                &device_;
	vk::Extent2D                    extent_;
	std::vector<backend::Image>     images_;
	std::vector<backend::ImageView> image_views_;
	std::vector<Attachment>         attachments_;

	std::vector<uint32_t> input_attachments_  = {};
	std::vector<uint32_t> output_attachments_ = {0};
};
}        // namespace rendering
}        // namespace xihe
