#include "free_camera.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include "scene_graph/components/camera.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"

namespace xihe::sg
{
const float FreeCamera::TOUCH_DOWN_MOVE_FORWARD_WAIT_TIME = 2.0f;

const float FreeCamera::ROTATION_MOVE_WEIGHT = 0.1f;

const float FreeCamera::KEY_ROTATION_MOVE_WEIGHT = 0.5f;

const float FreeCamera::TRANSLATION_MOVE_WEIGHT = 3.0f;

const float FreeCamera::TRANSLATION_MOVE_STEP = 50.0f;

const uint32_t FreeCamera::TRANSLATION_MOVE_SPEED = 4;

FreeCamera::FreeCamera(Node &node) :
    NodeScript{node, "FreeCamera"}
{}

void FreeCamera::update(float delta_time)
{
	glm::vec3 delta_translation(0.0f, 0.0f, 0.0f);
	glm::vec3 delta_rotation(0.0f, 0.0f, 0.0f);

	float mul_translation = speed_multiplier_;

	if (key_pressed_[KeyCode::W])
	{
		delta_translation.z -= TRANSLATION_MOVE_STEP;
	}
	if (key_pressed_[KeyCode::S])
	{
		delta_translation.z += TRANSLATION_MOVE_STEP;
	}
	if (key_pressed_[KeyCode::A])
	{
		delta_translation.x -= TRANSLATION_MOVE_STEP;
	}
	if (key_pressed_[KeyCode::D])
	{
		delta_translation.x += TRANSLATION_MOVE_STEP;
	}
	if (key_pressed_[KeyCode::Q])
	{
		delta_translation.y -= TRANSLATION_MOVE_STEP;
	}
	if (key_pressed_[KeyCode::E])
	{
		delta_translation.y += TRANSLATION_MOVE_STEP;
	}
	if (key_pressed_[KeyCode::LeftControl])
	{
		mul_translation *= (1.0f * TRANSLATION_MOVE_SPEED);
	}
	if (key_pressed_[KeyCode::LeftShift])
	{
		mul_translation *= (1.0f / TRANSLATION_MOVE_SPEED);
	}

	if (key_pressed_[KeyCode::I])
	{
		delta_rotation.x += KEY_ROTATION_MOVE_WEIGHT;
	}
	if (key_pressed_[KeyCode::K])
	{
		delta_rotation.x -= KEY_ROTATION_MOVE_WEIGHT;
	}
	if (key_pressed_[KeyCode::J])
	{
		delta_rotation.y += KEY_ROTATION_MOVE_WEIGHT;
	}
	if (key_pressed_[KeyCode::L])
	{
		delta_rotation.y -= KEY_ROTATION_MOVE_WEIGHT;
	}

	if (mouse_button_pressed_[MouseButton::Left] && mouse_button_pressed_[MouseButton::Right])
	{
		delta_rotation.z += TRANSLATION_MOVE_WEIGHT * mouse_move_delta_.x;
	}
	else if (mouse_button_pressed_[MouseButton::Right])
	{
		delta_rotation.x -= ROTATION_MOVE_WEIGHT * mouse_move_delta_.y;
		delta_rotation.y -= ROTATION_MOVE_WEIGHT * mouse_move_delta_.x;
	}
	else if (mouse_button_pressed_[MouseButton::Left])
	{
		delta_translation.x += TRANSLATION_MOVE_WEIGHT * mouse_move_delta_.x;
		delta_translation.y += TRANSLATION_MOVE_WEIGHT * -mouse_move_delta_.y;
	}

	if (touch_pointer_pressed_[0])
	{
		delta_rotation.x -= ROTATION_MOVE_WEIGHT * touch_move_delta_.y;
		delta_rotation.y -= ROTATION_MOVE_WEIGHT * touch_move_delta_.x;

		if (touch_pointer_time_ > TOUCH_DOWN_MOVE_FORWARD_WAIT_TIME)
		{
			delta_translation.z -= TRANSLATION_MOVE_STEP;
		}
		else
		{
			touch_pointer_time_ += delta_time;
		}
	}

	delta_translation *= mul_translation * delta_time;
	delta_rotation *= delta_time;

	// Only re-calculate the transform if it's changed
	if (delta_rotation != glm::vec3(0.0f, 0.0f, 0.0f) || delta_translation != glm::vec3(0.0f, 0.0f, 0.0f))
	{
		auto &transform = get_node().get_component<Transform>();

		glm::quat qx = glm::angleAxis(delta_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::quat qy = glm::angleAxis(delta_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::quat orientation = glm::normalize(qy * transform.get_rotation() * qx);

		transform.set_translation(transform.get_translation() + delta_translation * glm::conjugate(orientation));
		transform.set_rotation(orientation);
	}

	mouse_move_delta_ = {};
	touch_move_delta_ = {};
}

void FreeCamera::input_event(const InputEvent &input_event)
{
	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = static_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_action() == KeyAction::Down ||
		    key_event.get_action() == KeyAction::Repeat)
		{
			key_pressed_[key_event.get_code()] = true;
		}
		else
		{
			key_pressed_[key_event.get_code()] = false;
		}
	}
	else if (input_event.get_source() == EventSource::Mouse)
	{
		const auto &mouse_button = static_cast<const MouseButtonInputEvent &>(input_event);

		glm::vec2 mouse_pos{std::floor(mouse_button.get_pos_x()), std::floor(mouse_button.get_pos_y())};

		if (mouse_button.get_action() == MouseAction::Down)
		{
			mouse_button_pressed_[mouse_button.get_button()] = true;
		}

		if (mouse_button.get_action() == MouseAction::Up)
		{
			mouse_button_pressed_[mouse_button.get_button()] = false;
		}

		if (mouse_button.get_action() == MouseAction::Move)
		{
			mouse_move_delta_ = mouse_pos - mouse_last_pos_;

			mouse_last_pos_ = mouse_pos;
		}
	}
	else if (input_event.get_source() == EventSource::Touchscreen)
	{
		const auto &touch_event = static_cast<const TouchInputEvent &>(input_event);

		glm::vec2 touch_pos{std::floor(touch_event.get_pos_x()), std::floor(touch_event.get_pos_y())};

		if (touch_event.get_action() == TouchAction::Down)
		{
			touch_pointer_pressed_[touch_event.get_pointer_id()] = true;

			touch_last_pos_ = touch_pos;
		}

		if (touch_event.get_action() == TouchAction::Up)
		{
			touch_pointer_pressed_[touch_event.get_pointer_id()] = false;

			touch_pointer_time_ = 0.0f;
		}

		if (touch_event.get_action() == TouchAction::Move && touch_event.get_pointer_id() == 0)
		{
			touch_move_delta_ = touch_pos - touch_last_pos_;

			touch_last_pos_ = touch_pos;
		}
	}
}

void FreeCamera::resize(uint32_t width, uint32_t height)
{
	auto &camera_node = get_node();

	if (camera_node.has_component<Camera>())
	{
		if (auto camera = dynamic_cast<PerspectiveCamera *>(&camera_node.get_component<Camera>()))
		{
			camera->set_aspect_ratio(static_cast<float>(width) / height);
		}
	}
}

void FreeCamera::set_speed_multiplier(float speed_multiplier)
{
	speed_multiplier_ = speed_multiplier;
}
}
