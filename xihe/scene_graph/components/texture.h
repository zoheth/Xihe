#pragma once

#include "scene_graph/component.h"
#include "scene_graph/components/sampler.h"

namespace xihe::sg
{
class Image;

class Texture : public Component
{
  public:
	Texture(const std::string &name);

	Texture(Texture &&other) = default;

	virtual ~Texture() = default;

	virtual std::type_index get_type() override;

	void set_image(Image &image);

	Image *get_image();

	void set_sampler(Sampler &sampler);

	Sampler *get_sampler();

	vk::DescriptorImageInfo get_descriptor_image_info() const;

  private:
	Image *image_{nullptr};

	Sampler *sampler_{nullptr};
};

class BindlessTextures : public Component
{
  public:
	BindlessTextures(const std::string &name);

	BindlessTextures(BindlessTextures &&other) = default;

	virtual ~BindlessTextures() = default;

	virtual std::type_index get_type() override;

	std::vector<Texture *> get_textures();

	void add_texture(std::unique_ptr<Texture> &&texture);

	uint32_t get_texture_index(const std::string &name);

  private:
	std::vector<std::unique_ptr<Texture>> textures_;

	std::unordered_map<std::string, uint32_t> texture_map_;
};
}        // namespace xihe::sg