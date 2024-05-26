#pragma once

#include "backend/sampler.h"
#include "scene_graph/component.h"

namespace xihe::sg
{
class Sampler : public Component
{
  public:
	Sampler(const std::string &name, backend::Sampler &&vk_sampler);

	Sampler(Sampler &&other) = default;

	virtual ~Sampler() = default;

	virtual std::type_index get_type() override;

	backend::Sampler vk_sampler_;
};
}
