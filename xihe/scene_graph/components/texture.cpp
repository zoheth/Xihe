#include "texture.h"

#include "scene_graph/components/sampler.h"
#include "scene_graph/components/image.h"

namespace xihe::sg
{
Texture::Texture(const std::string &name) :
    Component{name}
{}

std::type_index Texture::get_type()
{
	return typeid(Texture);
}

void Texture::set_image(Image &i)
{
	image_ = &i;
}

Image *Texture::get_image()
{
	return image_;
}

void Texture::set_sampler(Sampler &s)
{
	sampler_ = &s;
}

Sampler *Texture::get_sampler()
{
	assert(sampler_ && "Texture has no sampler");
	return sampler_;
}
}
