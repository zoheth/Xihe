#pragma once

#include "backend/vulkan_resource.h"

namespace xihe::backend
{
class Sampler : public VulkanResource<vk::Sampler>
{
public:
	Sampler(backend::Device &device, const vk::SamplerCreateInfo &info);

	Sampler(const Sampler &) = delete;

	Sampler(Sampler &&sampler) noexcept;

	~Sampler() override;

	Sampler &operator=(const Sampler &) = delete;

	Sampler &operator=(Sampler &&) = delete;
};
}
