#pragma once

#include "backend/device.h"
#include "scene_graph/component.h"
#include <vulkan/vulkan.hpp>

namespace xihe::sg
{
bool is_astc(vk::Format format);

/**
 * @brief Mipmap information
 */
struct Mipmap
{
	uint32_t     level  = 0;                /// Mipmap level
	uint32_t     offset = 0;                /// Byte offset used for uploading
	vk::Extent3D extent = {0, 0, 0};        /// Width depth and height of the mipmap
};

class Image : public sg::Component
{
  public:
	Image(const std::string &name, std::vector<uint8_t> &&data = {}, std::vector<sg::Mipmap> &&mipmaps = {{}});
	virtual ~Image() = default;

  public:
	/**
	 * @brief Type of content held in image.
	 * This helps to steer the image loaders when deciding what the format should be.
	 * Some image containers don't know whether the data they contain is sRGB or not.
	 * Since most applications save color images in sRGB, knowing that an image
	 * contains color data helps us to better guess its format when unknown.
	 */
	enum ContentType
	{
		kUnknown,
		kColor,
		kOther
	};

	static std::unique_ptr<sg::Image> load(const std::string &name, const std::string &uri, ContentType content_type);

	// from Component
	virtual std::type_index get_type() override;

	void                                                        clear_data();
	void                                                        coerce_format_to_srgb();
	void                                                        create_vk_image(backend::Device &device, vk::ImageViewType image_view_type = vk::ImageViewType::e2D, vk::ImageCreateFlags flags = {});
	void                                                        generate_mipmaps();
	const std::vector<uint8_t>                                 &get_data() const;
	const vk::Extent3D                                         &get_extent() const;
	vk::Format                                                  get_format() const;
	const uint32_t                                              get_layers() const;
	const std::vector<sg::Mipmap> &get_mipmaps() const;
	const std::vector<std::vector<vk::DeviceSize>>             &get_offsets() const;
	const backend::Image                                  &get_vk_image() const;
	const backend::ImageView                              &get_vk_image_view() const;

  protected:
	sg::Mipmap              &get_mipmap(size_t index);
	std::vector<uint8_t>                                 &get_mut_data();
	std::vector<sg::Mipmap> &get_mut_mipmaps();
	void                                                  set_data(const uint8_t *raw_data, size_t size);
	void                                                  set_depth(uint32_t depth);
	void                                                  set_format(vk::Format format);
	void                                                  set_format(VkFormat format);
	void                                                  set_height(uint32_t height);
	void                                                  set_layers(uint32_t layers);
	void                                                  set_offsets(const std::vector<std::vector<vk::DeviceSize>> &offsets);
	void                                                  set_width(uint32_t width);

  private:
	std::vector<uint8_t>                                 data;
	vk::Format                                           format = vk::Format::eUndefined;
	uint32_t                                             layers = 1;
	std::vector<sg::Mipmap> mipmaps{{}};
	std::vector<std::vector<vk::DeviceSize>>             offsets;        // Offsets stored like offsets[array_layer][mipmap_layer]
	std::unique_ptr<backend::Image>                 vk_image;
	std::unique_ptr<backend::ImageView>             vk_image_view;
};
}
