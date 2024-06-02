#pragma once

#include "common/vk_common.h"
#include "scene_graph/components/image.h"

namespace xihe::sg
{
struct BlockDim
{
	uint8_t x;
	uint8_t y;
	uint8_t z;
};

class Astc : public Image
{
  public:
	Astc(const Image &image);

	Astc(const std::string &name, const std::vector<uint8_t> &data);

	virtual ~Astc() = default;

  private:
	/**
	 * @brief Decodes ASTC data
	 */
	void decode(BlockDim blockdim, vk::Extent3D extent, const uint8_t *data, uint32_t size);

	void init();
};
}