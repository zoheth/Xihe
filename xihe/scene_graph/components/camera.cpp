#include "camera.h"

#include "scene_graph/components/transform.h"
#include "scene_graph/scripts/free_camera.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"

namespace xihe::sg
{

Camera::Camera(const std::string &name) :
    Component{name}
{}

std::type_index Camera::get_type()
{
	return typeid(Camera);
}

glm::mat4 Camera::get_view()
{
	if (!node_)
	{
		throw std::runtime_error{"Camera component is not attached to a node"};
	}

	auto &transform = node_->get_component<Transform>();
	return glm::inverse(transform.get_world_matrix());
}

void Camera::set_node(Node &n)
{
	node_ = &n;
}

Node *Camera::get_node()
{
	return node_;
}

const glm::mat4 Camera::get_pre_rotation()
{
	return pre_rotation_;
}

void Camera::set_pre_rotation(const glm::mat4 &pr)
{
	pre_rotation_ = pr;
}


PerspectiveCamera::PerspectiveCamera(const std::string &name) :
    Camera{name}
{}

void PerspectiveCamera::set_field_of_view(float new_fov)
{
	fov_ = new_fov;
}

float PerspectiveCamera::get_far_plane() const
{
	return far_plane_;
}

void PerspectiveCamera::set_far_plane(float zfar)
{
	far_plane_ = zfar;
}

float PerspectiveCamera::get_near_plane() const
{
	return near_plane_;
}

void PerspectiveCamera::set_near_plane(float znear)
{
	near_plane_ = znear;
}

void PerspectiveCamera::set_aspect_ratio(float new_aspect_ratio)
{
	aspect_ratio_ = new_aspect_ratio;
}

float PerspectiveCamera::get_field_of_view()
{
	return fov_;
}

float PerspectiveCamera::get_aspect_ratio()
{
	return aspect_ratio_;
}

glm::mat4 PerspectiveCamera::get_projection()
{
	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	return glm::perspective(fov_, aspect_ratio_, far_plane_, near_plane_);
}

Node &add_free_camera(Scene &scene, const std::string &node_name, vk::Extent2D extent)
{
	auto camera_node = scene.find_node(node_name);

	if (!camera_node)
	{
		LOGW("Camera node `{}` not found. Looking for `default_camera` node.", node_name.c_str());

		camera_node = scene.find_node("default_camera");
	}

	if (!camera_node)
	{
		throw std::runtime_error("Camera node with name `" + node_name + "` not found.");
	}

	if (!camera_node->has_component<sg::Camera>())
	{
		throw std::runtime_error("No camera component found for `" + node_name + "` node.");
	}

	auto free_camera_script = std::make_unique<sg::FreeCamera>(*camera_node);

	free_camera_script->resize(extent.width, extent.height);

	scene.add_component(std::move(free_camera_script), *camera_node);

	return *camera_node;
}
}
