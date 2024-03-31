#pragma once
#include <sstream>
#include <vector>

namespace xihe::backend
{
	class ResourceRecord
	{
	public:

	private:
		std::ostringstream stream_;

		std::vector<size_t> shader_module_indices_;
		std::vector<size_t> pipeline_layout_indices_;
		std::vector<size_t> render_pass_indices_;
		std::vector<size_t> graphics_pipeline_indices_;
	};
}
