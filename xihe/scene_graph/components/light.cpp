#include "light.h"

#include <memory>

#include "scene_graph/node.h"
#include "scene_graph/scene.h"

namespace xihe::sg
{
Light::Light(const std::string &name) :
    Component{name}
{}

std::type_index Light::get_type()
{
	return typeid(Light);
}

void Light::set_node(Node &n)
{
	node = &n;
}

Node *Light::get_node() const
{
	return node;
}

void Light::set_light_type(const LightType &type)
{
	this->light_type = type;
}

const LightType &Light::get_light_type() const
{
	return light_type;
}

void Light::set_properties(const LightProperties &properties)
{
	this->properties = properties;
}

const LightProperties &Light::get_properties() const
{
	return properties;
}

sg::Light & add_light(sg::Scene &scene, sg::LightType type, const glm::vec3 &position, const glm::quat &rotation, const sg::LightProperties &props, sg::Node *parent_node)
{
	auto light_ptr = std::make_unique<sg::Light>("light");
	auto node      = std::make_unique<sg::Node>(-1, "light node");

	if (parent_node)
	{
		node->set_parent(*parent_node);
	}

	light_ptr->set_node(*node);
	light_ptr->set_light_type(type);
	light_ptr->set_properties(props);

	auto &t = node->get_transform();
	t.set_translation(position);
	t.set_rotation(rotation);

	// Storing the light component because the unique_ptr will be moved to the scene
	auto &light = *light_ptr;

	node->set_component(light);
	scene.add_child(*node);
	scene.add_component(std::move(light_ptr));
	scene.add_node(std::move(node));

	return light;
}

sg::Light & add_point_light(sg::Scene &scene, const glm::vec3 &position, const sg::LightProperties &props, sg::Node *parent_node)
{
	return add_light(scene, sg::LightType::kPoint, position, {}, props, parent_node);
}

sg::Light & add_directional_light(sg::Scene &scene, const glm::quat &rotation, const sg::LightProperties &props, sg::Node *parent_node)
{
	return add_light(scene, sg::LightType::kDirectional, {}, rotation, props, parent_node);
}

sg::Light & add_spot_light(sg::Scene &scene, const glm::vec3 &position, const glm::quat &rotation, const sg::LightProperties &props, sg::Node *parent_node)
{
	return add_light(scene, sg::LightType::kSpot, position, rotation, props, parent_node);
}
}
