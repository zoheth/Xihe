#include "command_pool.h"

namespace xihe::backend
{
Device &CommandPool::get_device()
{
	return device_;
}

vk::CommandPool CommandPool::get_handle() const
{
	return handle_;
}

uint32_t CommandPool::get_queue_family_index() const
{
	return queue_family_index_;
}

rendering::RenderFrame *CommandPool::get_render_frame()
{
	return render_frame_;
}

CommandBuffer::ResetMode CommandPool::get_reset_mode() const
{
	return reset_mode_;
}

size_t CommandPool::get_thread_index() const
{
	return thread_index_;
}

CommandBuffer &CommandPool::request_command_buffer(vk::CommandBufferLevel level)
{
	if (level == vk::CommandBufferLevel::ePrimary)
	{
		if (active_primary_command_buffer_count_ < primary_command_buffers_.size())
		{
			return *primary_command_buffers_[active_primary_command_buffer_count_++];
		}
		primary_command_buffers_.emplace_back(std::make_unique<CommandBuffer>(*this, level));

		return *primary_command_buffers_[active_primary_command_buffer_count_++];
		
	}
	else
	{
		if (active_secondary_command_buffer_count_ < secondary_command_buffers_.size())
		{
			return *secondary_command_buffers_[active_secondary_command_buffer_count_++];
		}
		secondary_command_buffers_.emplace_back(std::make_unique<CommandBuffer>(*this, level));
		return *secondary_command_buffers_[active_secondary_command_buffer_count_++];
		
	}
}
}        // namespace xihe::backend
