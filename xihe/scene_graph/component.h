#pragma once

#include <string>
#include <typeindex>

namespace xihe::sg
{
class Node;

/// @brief A generic class which can be used by nodes.
class Component
{
  public:
	Component() = default;

	Component(std::string name);

	Component(Component &&other) = default;

	virtual ~Component() = default;

	const std::string &get_name() const;

	virtual std::type_index get_type() = 0;

  private:
	std::string name_;
};
}
