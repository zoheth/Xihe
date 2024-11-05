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

Node &add_free_camera(Scene &scene, const std::string &node_name, vk::Extent2D extent, float speed)
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

	free_camera_script->set_speed_multiplier(speed);

	scene.add_component(std::move(free_camera_script), *camera_node);

	return *camera_node;
}

OrthographicCamera::OrthographicCamera(const std::string &name) :
    Camera{name}
{}

OrthographicCamera::OrthographicCamera(const std::string &name, float left, float right, float bottom, float top, float near_plane, float far_plane) :
    Camera{name},
    left_{left},
    right_{right},
    top_{top},
    bottom_{bottom},
    near_plane_{near_plane},
    far_plane_{far_plane}
{
}

void OrthographicCamera::set_bounds(glm::vec3 min_bounds, glm::vec3 max_bounds)
{
	left_       = min_bounds.x;
	right_      = max_bounds.x;
	bottom_     = min_bounds.y;
	top_        = max_bounds.y;
	near_plane_ = min_bounds.z;
	far_plane_  = max_bounds.z;
}

void OrthographicCamera::set_left(float new_left)
{
	left_ = new_left;
}

float OrthographicCamera::get_left() const
{
	return left_;
}

void OrthographicCamera::set_right(float new_right)
{
	right_ = new_right;
}

float OrthographicCamera::get_right() const
{
	return right_;
}

void OrthographicCamera::set_bottom(float new_bottom)
{
	bottom_ = new_bottom;
}

float OrthographicCamera::get_bottom() const
{
	return bottom_;
}

void OrthographicCamera::set_top(float new_top)
{
	top_ = new_top;
}

float OrthographicCamera::get_top() const
{
	return top_;
}

void OrthographicCamera::set_near_plane(float new_near_plane)
{
	near_plane_ = new_near_plane;
}

float OrthographicCamera::get_near_plane() const
{
	return near_plane_;
}

void OrthographicCamera::set_far_plane(float new_far_plane)
{
	far_plane_ = new_far_plane;
}

float OrthographicCamera::get_far_plane() const
{
	return far_plane_;
}

glm::mat4 OrthographicCamera::get_projection()
{
	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	return glm::ortho(left_, right_, bottom_, top_, far_plane_, near_plane_);
}

}
