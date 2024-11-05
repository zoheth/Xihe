#pragma once

#include <unordered_map>

#include "common/error.h"
#include "common/glm_common.h"
#include "scene_graph/script.h"
namespace xihe::sg
{
	class FreeCamera:public NodeScript
	{
	  public:
	    static const float TOUCH_DOWN_MOVE_FORWARD_WAIT_TIME;

	    static const float ROTATION_MOVE_WEIGHT;

	    static const float KEY_ROTATION_MOVE_WEIGHT;

	    static const float TRANSLATION_MOVE_WEIGHT;

	    static const float TRANSLATION_MOVE_STEP;

	    static const uint32_t TRANSLATION_MOVE_SPEED;

	    FreeCamera(Node &node);

	    virtual ~FreeCamera() = default;

		void update(float delta_time) override;

		void input_event(const InputEvent &input_event) override;

	    void resize(uint32_t width, uint32_t height) override;

		void set_speed_multiplier(float speed_multiplier);

	  private:
	    float speed_multiplier_{3.0f};

	    glm::vec2 mouse_move_delta_{0.0f};

	    glm::vec2 mouse_last_pos_{0.0f};

	    glm::vec2 touch_move_delta_{0.0f};

	    glm::vec2 touch_last_pos_{0.0f};

	    float touch_pointer_time_{0.0f};

	    std::unordered_map<KeyCode, bool> key_pressed_;

	    std::unordered_map<MouseButton, bool> mouse_button_pressed_;

	    std::unordered_map<int32_t, bool> touch_pointer_pressed_;
	};
}
