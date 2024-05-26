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

  private:
	Image *image_{nullptr};

	Sampler *sampler_{nullptr};
};
}