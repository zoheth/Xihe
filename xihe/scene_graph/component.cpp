#include "component.h"

#include "node.h"

namespace xihe::sg
{
Component::Component(std::string name) :
    name_{std::move(name)}
{}

const std::string &Component::get_name() const
{
	return name_;
}
}
