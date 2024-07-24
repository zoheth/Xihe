#include "rdg_builder.h"

namespace xihe::rendering
{

void set_viewport_and_scissor(backend::CommandBuffer const &command_buffer, vk::Extent2D const &extent)
{
	command_buffer.get_handle().setViewport(0, {{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f}});
	command_buffer.get_handle().setScissor(0, vk::Rect2D({}, extent));
}

void SecondaryDrawTask::init(backend::CommandBuffer *command_buffer, RdgPass const *pass, uint32_t subpass_index)
{
	this->command_buffer = command_buffer;
	this->pass           = pass;
	this->subpass_index  = subpass_index;
}

void SecondaryDrawTask::ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
{
	const RenderTarget *render_target = pass->get_render_target();

	command_buffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &pass->get_render_pass(), &pass->get_framebuffer(), subpass_index);
	command_buffer->init_state(subpass_index);
	set_viewport_and_scissor(*command_buffer, render_target->get_extent());
	pass->draw_subpass(*command_buffer, *render_target, subpass_index);
	command_buffer->end();
}

RdgBuilder::RdgBuilder(RenderContext &render_context) :
    render_context_{render_context}
{
}

void RdgBuilder::execute() const
{
	backend::CommandBuffer &command_buffer = render_context_.request_graphics_command_buffer(backend::CommandBuffer::ResetMode::kResetPool,
	                                                vk::CommandBufferLevel::ePrimary, 0);

	command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

#if 1
	for (auto &rdg_name : pass_order_)
	{
		auto &rdg_pass = rdg_passes_.at(rdg_name);
		RenderTarget *render_target = rdg_pass->get_render_target();

		set_viewport_and_scissor(command_buffer, render_target->get_extent());

		auto     image_infos                         = rdg_pass->get_descriptor_image_infos(*render_target);
		/*uint32_t first_bindless_descriptor_set_index = std::numeric_limits<uint32_t>::max();
		for (auto &image_info : image_infos)
		{
			auto index = render_context_.get_bindless_descriptor_set()->update(image_info);

			if (index < first_bindless_descriptor_set_index)
			{
				first_bindless_descriptor_set_index = index;
			}
		}*/
		// rdg_pass->prepare(command_buffer);
		rdg_pass->execute(command_buffer, *render_target, {});
	}
#elif 1

	enki::TaskScheduler scheduler;
	scheduler.Initialize();

	auto        reset_mode = backend::CommandBuffer::ResetMode::kResetPool;
	const auto &queue      = render_context_.get_device().get_suitable_graphics_queue();

	std::unordered_map<std::string, std::vector<SecondaryDrawTask>> rdg_pass_tasks;

	uint32_t thread_index = 1;

	for (auto &rdg_name : pass_order_)
	{
		auto &rdg_pass = rdg_passes_.at(rdg_name);
		// RenderTarget *render_target = rdg_pass->get_render_target();

		/*auto     image_infos                         = rdg_pass->get_descriptor_image_infos(*render_target);
		uint32_t first_bindless_descriptor_set_index = std::numeric_limits<uint32_t>::max();
		for (auto &image_info : image_infos)
		{
		    auto index = render_context_.get_bindless_descriptor_set()->update(image_info);

		    if (index < first_bindless_descriptor_set_index)
		    {
		        first_bindless_descriptor_set_index = index;
		    }
		}*/

		rdg_pass_tasks[rdg_name] = std::vector<SecondaryDrawTask>(rdg_pass->get_subpass_count());

		rdg_pass->prepare(command_buffer);

		for (uint32_t i = 0; i < rdg_pass->get_subpass_count(); ++i)
		{
			auto &secondary_command_buffer = render_context_.get_active_frame().request_command_buffer(queue,
			                                                                                           reset_mode,
			                                                                                           vk::CommandBufferLevel::eSecondary,
			                                                                                           thread_index);

			rdg_pass->set_thread_index(i, thread_index);

			rdg_pass_tasks[rdg_name][i].init(&secondary_command_buffer, rdg_pass.get(), i);

			scheduler.AddTaskSetToPipe(&rdg_pass_tasks[rdg_name][i]);
			thread_index++;
		}
	}

	uint32_t pass_index = 0;

	for (auto &rdg_name : pass_order_)
	{
		auto &rdg_pass = rdg_passes_.at(rdg_name);

		std::vector<backend::CommandBuffer *> secondary_command_buffers;
		for (uint32_t i = 0; i < rdg_pass->get_subpass_count(); ++i)
		{
			scheduler.WaitforTask(&rdg_pass_tasks[rdg_name][i]);
			secondary_command_buffers.push_back(rdg_pass_tasks[rdg_name][i].command_buffer);
		}

		rdg_pass->execute(command_buffer, *rdg_pass->get_render_target(), secondary_command_buffers);

		pass_index++;
	}

#endif

	command_buffer.end();

	render_context_.submit(command_buffer);
}

}        // namespace xihe::rendering
