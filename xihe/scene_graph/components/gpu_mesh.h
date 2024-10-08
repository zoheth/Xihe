#pragma once

#include "backend/buffer.h"
#include "backend/shader_module.h"
#include "scene_graph/components/sub_mesh.h"

namespace xihe::sg
{
class Material;

class GpuMesh : public SubMesh
{
public:
	GpuMesh(const std::string &name = {});
  private:
	
};
}
