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

class OrthographicCamera : public Camera
{
  public:
	OrthographicCamera(const std::string &name);

	OrthographicCamera(const std::string &name, float left, float right, float bottom, float top, float near_plane, float far_plane);

	virtual ~OrthographicCamera() = default;

	void set_bounds(glm::vec3 min_bounds, glm::vec3 max_bounds);

	void set_left(float left);

	float get_left() const;

	void set_right(float right);

	float get_right() const;

	void set_bottom(float bottom);

	float get_bottom() const;

	void set_top(float top);

	float get_top() const;

	void set_near_plane(float near_plane);

	float get_near_plane() const;

	void set_far_plane(float far_plane);

	float get_far_plane() const;

	virtual glm::mat4 get_projection() override;

  private:
	float left_{-1.0f};

	float right_{1.0f};

	float bottom_{-1.0f};

	float top_{1.0f};

	float near_plane_{0.0f};

	float far_plane_{1.0f};
};

Node &add_free_camera(Scene &scene, const std::string &node_name, vk::Extent2D extent, float speed=3.0f);

}
