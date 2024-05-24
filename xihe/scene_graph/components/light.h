#pragma once
#include "common/glm_common.h"
#include "scene_graph/component.h"

namespace xihe::sg
{
enum LightType
{
	kDirectional = 0,
	kPoint       = 1,
	kSpot        = 2,
	kMax
};

struct LightProperties
{
	glm::vec3 direction{0.0f, 0.0f, -1.0f};

	glm::vec3 color{1.0f, 1.0f, 1.0f};

	float intensity{1.0f};

	float range{0.0f};

	float inner_cone_angle{0.0f};

	float outer_cone_angle{0.0f};
};

class Light : public Component
{
  public:
	Light(const std::string &name);

	Light(Light &&other) = default;

	virtual ~Light() = default;

	virtual std::type_index get_type() override;

	void set_node(Node &node);

	Node *get_node() const;

	void set_light_type(const LightType &type);

	const LightType &get_light_type() const;

	void set_properties(const LightProperties &properties);

	const LightProperties &get_properties() const;

  private:
	Node *node{nullptr};

	LightType light_type;

	LightProperties properties;
};

}
