#include "sampler.h"

namespace xihe::sg
{
Sampler::Sampler(const std::string &name, backend::Sampler &&vk_sampler) :
    Component{name},
    vk_sampler_{std::move(vk_sampler)}
{}

std::type_index Sampler::get_type()
{
	return typeid(Sampler);
}
}
