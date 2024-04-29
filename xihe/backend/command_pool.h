#pragma once
#include <cstdint>

#include "backend/command_buffer.h"

namespace xihe
{
namespace rendering
{
class RenderFrame;
}
namespace backend
{
class Device;

class CommandPool
{
  public:
	CommandPool(Device                  &device,
	            uint32_t                 queue_family_index,
	            rendering::RenderFrame  *render_frame = nullptr,
	            size_t                   thread_index = 0,
	            CommandBuffer::ResetMode reset_mode   = CommandBuffer::ResetMode::kResetPool);
	CommandPool(const CommandPool &) = delete;
	// CommandPool(CommandPool &&other) noexcept;
	~CommandPool();

	CommandPool &operator=(const CommandPool &) = delete;
	CommandPool &operator=(CommandPool &&)      = delete;

	Device                  &get_device();
	vk::CommandPool          get_handle() const;
	uint32_t                 get_queue_family_index() const;
	rendering::RenderFrame  *get_render_frame();
	CommandBuffer::ResetMode get_reset_mode() const;
	size_t                   get_thread_index() const;
	CommandBuffer           &request_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

	void reset_pool();

  private:
	void reset_command_buffers();

  private:
	Device                 &device_;
	vk::CommandPool         handle_{nullptr};
	rendering::RenderFrame *render_frame_{nullptr};
	size_t                  thread_index_{0};
	uint32_t                queue_family_index_{0};

	std::vector<std::unique_ptr<CommandBuffer>> primary_command_buffers_;
	std::vector<std::unique_ptr<CommandBuffer>> secondary_command_buffers_;

	uint32_t active_primary_command_buffer_count_{0};
	uint32_t active_secondary_command_buffer_count_{0};

	CommandBuffer::ResetMode reset_mode_{CommandBuffer::ResetMode::kResetPool};
};
}        // namespace backend
}        // namespace xihe