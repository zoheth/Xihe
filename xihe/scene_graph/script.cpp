#include "script.h"

namespace xihe::sg
{
Script::Script(const std::string &name) :
    Component{name}
{}

std::type_index Script::get_type()
{
	return typeid(Script);
}

void Script::input_event(const InputEvent & /*input_event*/)
{
}

void Script::resize(uint32_t /*width*/, uint32_t /*height*/)
{
}

NodeScript::NodeScript(Node &node, const std::string &name) :
    Script{name},
    node_{node}
{
}

Node &NodeScript::get_node()
{
	return node_;
}
}
