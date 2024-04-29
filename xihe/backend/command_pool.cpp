#include "command_pool.h"

#include "backend/device.h"

namespace xihe::backend
{
CommandPool::CommandPool(Device &device, uint32_t queue_family_index, rendering::RenderFrame *render_frame, size_t thread_index, CommandBuffer::ResetMode reset_mode) :
    device_{device},
    render_frame_{render_frame},
    thread_index_{thread_index},
    reset_mode_{reset_mode}
{
	vk::CommandPoolCreateFlags flags;
	switch (reset_mode)
	{
		case CommandBuffer::ResetMode::kResetIndividually:
		case CommandBuffer::ResetMode::kAlwaysAllocate:
			flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			break;
		case CommandBuffer::ResetMode::kResetPool:
		default:
			flags = vk::CommandPoolCreateFlagBits::eTransient;
			break;
	}

	vk::CommandPoolCreateInfo command_pool_create_info(flags, queue_family_index);

	handle_ = device.get_handle().createCommandPool(command_pool_create_info);
}

CommandPool::~CommandPool()
{
	primary_command_buffers_.clear();
	secondary_command_buffers_.clear();

	if (handle_)
	{
		device_.get_handle().destroyCommandPool(handle_);
	}
}

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

void CommandPool::reset_pool()
{
	switch (reset_mode_)
	{
		case CommandBuffer::ResetMode::kResetIndividually:
			reset_command_buffers();
			break;
		case CommandBuffer::ResetMode::kResetPool:
			device_.get_handle().resetCommandPool(handle_);
			reset_command_buffers();
			break;

		case CommandBuffer::ResetMode::kAlwaysAllocate:
			primary_command_buffers_.clear();
			active_primary_command_buffer_count_ = 0;
			secondary_command_buffers_.clear();
			active_secondary_command_buffer_count_ = 0;
			break;

		default:
			throw std::runtime_error("Unknown reset mode for command pools");
	}
}

void CommandPool::reset_command_buffers()
{
	for (auto &command_buffer : primary_command_buffers_)
	{
		command_buffer->reset(reset_mode_);
	}
	active_primary_command_buffer_count_ = 0;

	for (auto &command_buffer : secondary_command_buffers_)
	{
		command_buffer->reset(reset_mode_);
	}
	active_secondary_command_buffer_count_ = 0;	
}
}        // namespace xihe::backend
