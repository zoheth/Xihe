#pragma once

#include "platform/input_events.h"
#include "scene_graph/component.h"

namespace xihe::sg
{
class Script : public Component
{
  public:
	Script(const std::string &name = "");

	~Script() override = default;

	virtual std::type_index get_type() override;


	virtual void update(float delta_time) = 0;

	virtual void input_event(const InputEvent &input_event);

	virtual void resize(uint32_t width, uint32_t height);
};

class NodeScript : public Script
{
  public:
	NodeScript(Node &node, const std::string &name = "");

	~NodeScript() override = default;

	Node &get_node();

  private:
	Node &node_;
};
}
