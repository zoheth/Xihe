#include "image.h"

namespace
{
inline vk::ImageType find_image_type(vk::Extent3D const &extent)
{
	switch (!!extent.width + !!extent.height + (1 < extent.depth))
	{
		case 1:
			return vk::ImageType::e1D;
		case 2:
			return vk::ImageType::e2D;
		case 3:
			return vk::ImageType::e3D;
		default:
			throw std::runtime_error("No image type found.");
			return vk::ImageType();
	}
}
}        // namespace

namespace xihe::backend
{
Image::Image(Device &device, vk::Image handle, const vk::Extent3D &extent, vk::Format format, vk::ImageUsageFlags image_usage, vk::SampleCountFlagBits sample_count) :
    VulkanResource{handle, &device}, type_{find_image_type(extent)}, extent_{extent}, format_{format}, usage_{image_usage}, sample_count_{sample_count}
{
	subresource_.mipLevel   = 1;
	subresource_.arrayLayer = 1;
}
}        // namespace xihe::backend
