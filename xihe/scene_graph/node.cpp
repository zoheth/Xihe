#include "node.h"

namespace xihe::sg
{
Node::Node(const size_t id, std::string name) :
    id_{id},
    name_{std::move(name)},
    transform_{*this}
{
	set_component(transform_);
}

size_t Node::get_id() const
{
	return id_;
}

const std::string &Node::get_name() const
{
	return name_;
}

void Node::set_parent(Node &p)
{
	parent_ = &p;

	transform_.invalidate_world_matrix();
}

Node *Node::get_parent() const
{
	return parent_;
}

void Node::add_child(Node &child)
{
	children_.push_back(&child);
}

const std::vector<Node *> &Node::get_children() const
{
	return children_;
}

void Node::set_component(Component &component)
{
	auto it = components_.find(component.get_type());

	if (it != components_.end())
	{
		it->second = &component;
	}
	else
	{
		components_.insert(std::make_pair(component.get_type(), &component));
	}
}

Component &Node::get_component(const std::type_index index) const
{
	return *components_.at(index);
}

bool Node::has_component(const std::type_index index) const
{
	return components_.contains(index);
}
}
