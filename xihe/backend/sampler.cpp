#include "sampler.h"

#include "device.h"

namespace xihe::backend
{
Sampler::Sampler(backend::Device &device, const vk::SamplerCreateInfo &info):
 VulkanResource<vk::Sampler>{device.get_handle().createSampler(info), &device}
{}

Sampler::Sampler(Sampler &&sampler) noexcept:
 VulkanResource{std::move(sampler)}
{}

Sampler::~Sampler()
{
	if (get_handle())
	{
		get_device().get_handle().destroySampler(get_handle());
	}
}
}
