#include "texture.h"

#include "scene_graph/components/image.h"
#include "scene_graph/components/sampler.h"

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

vk::DescriptorImageInfo Texture::get_descriptor_image_info() const
{
	return vk::DescriptorImageInfo{
	    sampler_->vk_sampler_.get_handle(),
	    image_->get_vk_image_view().get_handle(),
	    vk::ImageLayout::eShaderReadOnlyOptimal
	};
}

BindlessTextures::BindlessTextures(const std::string &name) :
    Component{name}
{}

std::type_index BindlessTextures::get_type()
{
	return typeid(BindlessTextures);
}

std::vector<Texture *> BindlessTextures::get_textures()
{
	std::vector<Texture *> result;
	result.resize(textures_.size());
	std::ranges::transform(textures_, result.begin(),
	                       [](const std::unique_ptr<Texture> &texture) -> Texture * {
		                       return texture.get();
	                       });
	return result;
}

void BindlessTextures::add_texture(std::unique_ptr<Texture> &&texture)
{
	textures_.push_back(std::move(texture));
	texture_map_.emplace(textures_.back()->get_name(), static_cast<uint32_t>(textures_.size() - 1));
}

uint32_t BindlessTextures::get_texture_index(const std::string &name)
{
	auto it = texture_map_.find(name);

	if (it == texture_map_.end())
	{
		throw std::runtime_error{"Texture not found: " + name};
	}

	return it->second;
}
}        // namespace xihe::sg
