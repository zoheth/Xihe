#pragma once

#include "backend/buffer.h"
#include "backend/shader_module.h"
#include "scene_graph/components/sub_mesh.h"

namespace xihe::sg
{
class Material;

struct GpuMeshletVertexData
{
	uint8_t    normal[4];
	uint8_t    tangent[4];
	uint16_t   uv_coords[2];
	float padding;
}; 

class GpuMesh : public SubMesh
{
public:
	GpuMesh(const std::string &name = {});
  private:
	
};
}
