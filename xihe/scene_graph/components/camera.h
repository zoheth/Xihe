#pragma once

#include "common/error.h"

#include "common/glm_common.h"

#include "common/helpers.h"
#include "scene_graph/component.h"

namespace xihe::sg
{
class Scene;

class Camera : public Component
{
  public:
	Camera(const std::string &name);

	virtual ~Camera() = default;

	virtual std::type_index get_type() override;

	virtual glm::mat4 get_projection() = 0;

	glm::mat4 get_view();

	void set_node(Node &node);

	Node *get_node();

	const glm::mat4 get_pre_rotation();

	void set_pre_rotation(const glm::mat4 &pre_rotation);

  private:
	Node *node_{nullptr};

	glm::mat4 pre_rotation_{1.0f};
};


class PerspectiveCamera : public Camera
{
  public:
	PerspectiveCamera(const std::string &name);

	virtual ~PerspectiveCamera() = default;

	void set_aspect_ratio(float aspect_ratio);

	void set_field_of_view(float fov);

	float get_far_plane() const;

	void set_far_plane(float zfar);

	float get_near_plane() const;

	void set_near_plane(float znear);

	float get_aspect_ratio();

	float get_field_of_view();

	virtual glm::mat4 get_projection() override;

  private:
	/**
	 * @brief Screen size aspect ratio
	 */
	float aspect_ratio_{1.0f};

	/**
	 * @brief Horizontal field of view in radians
	 */
	float fov_{glm::radians(60.0f)};

	float far_plane_{100.0};

	float near_plane_{0.1f};
};

Node &add_free_camera(Scene &scene, const std::string &node_name, vk::Extent2D extent);

}
