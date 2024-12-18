#include "mesh.h"

namespace xihe::sg
{
Mesh::Mesh(const std::string &name) :
    Component{name}
{}

void Mesh::update_bounds(const std::vector<glm::vec3> &vertex_data, const std::vector<uint16_t> &index_data)
{
	bounds_.update(vertex_data, index_data);
}

std::type_index Mesh::get_type()
{
	return typeid(Mesh);
}

const AABB &Mesh::get_bounds() const
{
	return bounds_;
}

void Mesh::add_submesh(SubMesh &submesh)
{
	submeshes_.push_back(&submesh);
}

void Mesh::add_mshader_mesh(MshaderMesh &mshader_mesh)
{
	mshader_meshes_.push_back(&mshader_mesh);
}

void Mesh::add_submesh_data(Material &material, MeshPrimitiveData &&primitive_data)
{
	submeshes_data_.push_back({&material, std::move(primitive_data)});
}

const std::vector<SubMesh *> &Mesh::get_submeshes() const
{
	return submeshes_;
}

const std::vector<MshaderMesh *> &Mesh::get_mshader_meshes() const
{
	return mshader_meshes_;
}

void Mesh::add_node(Node &node)
{
	nodes_.push_back(&node);
}

const std::vector<Node *> &Mesh::get_nodes() const
{
	return nodes_;
}
}