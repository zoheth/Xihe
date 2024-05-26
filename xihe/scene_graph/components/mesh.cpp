#include "mesh.h"

namespace xihe::sg
{
Mesh::Mesh(const std::string &name) :
    Component{name}
{}

void Mesh::update_bounds(const std::vector<glm::vec3> &vertex_data, const std::vector<uint16_t> &index_data)
{
	bounds.update(vertex_data, index_data);
}

std::type_index Mesh::get_type()
{
	return typeid(Mesh);
}

const AABB &Mesh::get_bounds() const
{
	return bounds;
}

void Mesh::add_submesh(SubMesh &submesh)
{
	submeshes.push_back(&submesh);
}

const std::vector<SubMesh *> &Mesh::get_submeshes() const
{
	return submeshes;
}

void Mesh::add_node(Node &node)
{
	nodes.push_back(&node);
}

const std::vector<Node *> &Mesh::get_nodes() const
{
	return nodes;
}
}