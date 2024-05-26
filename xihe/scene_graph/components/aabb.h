#pragma once

#include "common/glm_common.h"

#include "scene_graph/component.h"
#include "scene_graph/components/sub_mesh.h"

namespace xihe::sg
{
class AABB : public Component
{
  public:
	AABB();

	AABB(const glm::vec3 &min, const glm::vec3 &max);

	virtual ~AABB() = default;

	virtual std::type_index get_type() override;

	void update(const glm::vec3 &point);

	void update(const std::vector<glm::vec3> &vertex_data, const std::vector<uint16_t> &index_data);

	void transform(glm::mat4 &transform);

	glm::vec3 get_scale() const;

	glm::vec3 get_center() const;

	
	glm::vec3 get_min() const;

	glm::vec3 get_max() const;

	void reset();

  private:
	glm::vec3 min_;

	glm::vec3 max_;
};
}
