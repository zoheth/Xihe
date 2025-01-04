#pragma once
#include "scene_graph/components/image.h"

namespace xihe
{

struct VirtualTexture
{
	vk::Image texture_image = VK_NULL_HANDLE;
	vk::ImageView texture_image_view = VK_NULL_HANDLE;

	size_t width  = 0U;
	size_t height = 0U;

	std::unique_ptr<sg::Image> raw_data_image;
};
}
