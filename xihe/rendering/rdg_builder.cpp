#include "rdg_builder.h"

namespace xihe::rendering
{
RdgBuilder::RdgBuilder(RenderContext &render_context) :
    render_context_{render_context}
{
}

void RdgBuilder::execute(backend::CommandBuffer &command_buffer) const
{
	for (auto &[rdg_name, rdg_pass] : rdg_passes_)
	{
		RenderTarget *render_target = rdg_pass->get_render_target();

		set_viewport_and_scissor(command_buffer, render_target->get_extent());

		auto     image_infos                         = rdg_pass->get_descriptor_image_infos(*render_target);
		uint32_t first_bindless_descriptor_set_index = std::numeric_limits<uint32_t>::max();
		for (auto &image_info : image_infos)
		{
			auto index = render_context_.get_bindless_descriptor_set()->update(image_info);

			if (index < first_bindless_descriptor_set_index)
			{
				first_bindless_descriptor_set_index = index;
			}
		}

		rdg_pass->execute(command_buffer, *render_target);
	}
}

void RdgBuilder::set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D({}, extent));
}

}        // namespace xihe::rendering
