#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace xihe::sg
{

class Node;
class Component;
class SubMesh;


class Scene
{
private:
	std::string name;

	/// List of all the nodes
	std::vector<std::unique_ptr<Node>> nodes;

	Node *root{nullptr};

	std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> components;
};
}
