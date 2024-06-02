#pragma once
#include "rendering/subpasses/geometry_subpass.h"

#define MAX_FORWARD_LIGHT_COUNT 8

namespace xihe
{
namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg

namespace rendering
{
struct alignas(16) ForwardLights
{
	Light directional_lights[MAX_FORWARD_LIGHT_COUNT];
	Light point_lights[MAX_FORWARD_LIGHT_COUNT];
	Light spot_lights[MAX_FORWARD_LIGHT_COUNT];
};

class ForwardSubpass : public GeometrySubpass
{
  public:

	ForwardSubpass(RenderContext &render_context, backend::ShaderSource &&vertex_shader, backend::ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera);

	virtual ~ForwardSubpass() = default;

	virtual void prepare() override;

	virtual void draw(backend::CommandBuffer &command_buffer) override;
};
}

}