#include "cascade_script.h"

#include "scene_graph/components/light.h"

namespace xihe::sg
{
CascadeScript::CascadeScript(const std::string &name, sg::Scene &scene, sg::PerspectiveCamera &camera) :
    Script(name),
    camera_(camera)
{
	const auto       lights            = scene.get_components<sg::Light>();
	const sg::Light *directional_light = nullptr;
	for (auto &light : lights)
	{
		if (light->get_light_type() == sg::LightType::kDirectional)
		{
			directional_light = light;
			break;
		}
	}

	for (uint32_t index = 0; index < cascade_count_; ++index)
	{
		cascade_cameras_[index] = std::make_unique<sg::OrthographicCamera>("cascade_cameras_" + std::to_string(index) + std::to_string(cascade_count_));
		cascade_cameras_[index]->set_node(*directional_light->get_node());
	}

	calculate_cascade_split_depth();
}

void CascadeScript::update(float delta_time)
{
	glm::mat4              inverse_view_projection = glm::inverse(camera_.get_projection() * camera_.get_view());
	std::vector<glm::vec3> corners(8);

	for (uint32_t cascade_index = 0; cascade_index < cascade_count_; ++cascade_index)
	{
		for (uint32_t i = 0; i < 8; i++)
		{
			glm::vec4 homogenous_corner = glm::vec4(
			    (i & 1) ? 1.0f : -1.0f,
			    (i & 2) ? 1.0f : -1.0f,
			    (i & 4) ? cascade_splits_[cascade_index] : cascade_splits_[cascade_index + 1],
			    //(i & 4) ? 1.0f : 0.0f,
			    1.0f);

			glm::vec4 world_corner = inverse_view_projection * homogenous_corner;
			corners[i]             = glm::vec3(world_corner) / world_corner.w;
		}

		glm::mat4 light_view_mat = cascade_cameras_[cascade_index]->get_view();

		for (auto &corner : corners)
		{
			corner = light_view_mat * glm::vec4(corner, 1.0f);
		}

		glm::vec3 min_bounds = glm::vec3(FLT_MAX), max_bounds(FLT_MIN);

		for (auto &corner : corners)
		{
			// In vulkan, clip space has inverted Y and depth range, so we need to flip the Y and Z axis
			corner.y   = -corner.y;
			corner.z   = -corner.z;
			min_bounds = glm::min(min_bounds, corner);
			max_bounds = glm::max(max_bounds, corner);
		}

		cascade_cameras_[cascade_index]->set_bounds(min_bounds, max_bounds);
	}
}

void CascadeScript::calculate_cascade_split_depth(float lambda) const
{
	float n = camera_.get_near_plane();
	float f = camera_.get_far_plane();
	cascade_splits_.resize(cascade_count_ + 1);
	for (uint32_t index = 0; index < cascade_splits_.size(); ++index)
	{
		float i = static_cast<float>(index);
		float N = static_cast<float>(cascade_count_);

		// Calculate the logarithmic and linear depth
		float c_log = n * std::pow((f / n), i / N);
		float c_lin = n + (i / N) * (f - n);

		// Interpolate between logarithmic and linear depth using lambda
		float c = lambda * c_log + (1 - lambda) * c_lin;

		// Convert view space depth to clip space depth for Vulkan's [0, 1] range.
		// The near and far planes are inverted in the projection matrix for precision reasons,
		// so we invert them here as well to match that convention.
		cascade_splits_[index] = n / (n - f) -
		                         (f * n) / ((n - f) * c);
	}
}

sg::OrthographicCamera &CascadeScript::get_cascade_camera(uint32_t index) const
{
	return *cascade_cameras_[index];
}

std::vector<float> &CascadeScript::get_cascade_splits()
{
	return cascade_splits_;
}
}        // namespace xihe::sg
