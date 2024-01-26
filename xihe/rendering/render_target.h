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

private:
	backend::Device &device_;
	vk::Extent2D extent_;
	std::vector<backend::Image> images_;
	std::vector<backend::ImageView> image_views_;
	std::vector<Attachment> attachments_;

	std::vector<uint32_t> input_attachments_ = {};
	std::vector<uint32_t> output_attachments_ = {0};
};
}
}
