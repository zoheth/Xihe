#include "aabb.h"

namespace xihe::sg
{
AABB::AABB()
{
	reset();
}

AABB::AABB(const glm::vec3 &min, const glm::vec3 &max) :
    min_{min},
    max_{max}
{
}

std::type_index AABB::get_type()
{
	return typeid(AABB);
}

void AABB::update(const glm::vec3 &point)
{
	min_ = glm::min(min_, point);
	max_ = glm::max(max_, point);
}

void AABB::update(const std::vector<glm::vec3> &vertex_data, const std::vector<uint16_t> &index_data)
{
	// Check if submesh is indexed
	if (index_data.size() > 0)
	{
		// Update bounding box for each indexed vertex
		for (size_t index_id = 0; index_id < index_data.size(); index_id++)
		{
			update(vertex_data[index_data[index_id]]);
		}
	}
	else
	{
		// Update bounding box for each vertex
		for (size_t vertex_id = 0; vertex_id < vertex_data.size(); vertex_id++)
		{
			update(vertex_data[vertex_id]);
		}
	}
}

void AABB::transform(glm::mat4 &transform)
{
	min_ = max_ = glm::vec4(min_, 1.0f) * transform;

	// Update bounding box for the remaining 7 corners of the box
	update(glm::vec4(min_.x, min_.y, max_.z, 1.0f) * transform);
	update(glm::vec4(min_.x, max_.y, min_.z, 1.0f) * transform);
	update(glm::vec4(min_.x, max_.y, max_.z, 1.0f) * transform);
	update(glm::vec4(max_.x, min_.y, min_.z, 1.0f) * transform);
	update(glm::vec4(max_.x, min_.y, max_.z, 1.0f) * transform);
	update(glm::vec4(max_.x, max_.y, min_.z, 1.0f) * transform);
	update(glm::vec4(max_, 1.0f) * transform);
}

glm::vec3 AABB::get_scale() const
{
	return (max_ - min_);
}

glm::vec3 AABB::get_center() const
{
	return (min_ + max_) * 0.5f;
}

glm::vec3 AABB::get_min() const
{
	return min_;
}

glm::vec3 AABB::get_max() const
{
	return max_;
}

void AABB::reset()
{
	min_ = std::numeric_limits<glm::vec3>::max();

	max_ = std::numeric_limits<glm::vec3>::min();
}
}
