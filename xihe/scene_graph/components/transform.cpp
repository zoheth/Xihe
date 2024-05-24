#include "transform.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "scene_graph/node.h"

namespace xihe::sg
{
Transform::Transform(Node &n) :
    node_{n}
{
}

Node &Transform::get_node()
{
	return node_;
}

std::type_index Transform::get_type()
{
	return typeid(Transform);
}

void Transform::set_translation(const glm::vec3 &new_translation)
{
	translation_ = new_translation;

	invalidate_world_matrix();
}

void Transform::set_rotation(const glm::quat &new_rotation)
{
	rotation_ = new_rotation;

	invalidate_world_matrix();
}

void Transform::set_scale(const glm::vec3 &new_scale)
{
	scale_ = new_scale;

	invalidate_world_matrix();
}

const glm::vec3 &Transform::get_translation() const
{
	return translation_;
}

const glm::quat &Transform::get_rotation() const
{
	return rotation_;
}

const glm::vec3 &Transform::get_scale() const
{
	return scale_;
}

void Transform::set_matrix(const glm::mat4 &matrix)
{
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale_, rotation_, translation_, skew, perspective);

	invalidate_world_matrix();
}

glm::mat4 Transform::get_matrix() const
{
	return glm::translate(glm::mat4(1.0), translation_) *
	       glm::mat4_cast(rotation_) *
	       glm::scale(glm::mat4(1.0), scale_);
}

glm::mat4 Transform::get_world_matrix()
{
	update_world_transform();

	return world_matrix_;
}

void Transform::invalidate_world_matrix()
{
	update_world_matrix_ = true;
}

void Transform::update_world_transform()
{
	if (!update_world_matrix_)
	{
		return;
	}

	world_matrix_ = get_matrix();

	auto parent = node_.get_parent();

	if (parent)
	{
		auto &transform = parent->get_component<Transform>();
		world_matrix_    = transform.get_world_matrix() * world_matrix_;
	}

	update_world_matrix_ = false;
}
}
