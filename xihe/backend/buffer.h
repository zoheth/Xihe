#pragma once
#include <memory>

#include "allocated.h"

namespace xihe::backend
{
class Device;
class Buffer;
using BufferPtr = std::unique_ptr<Buffer>;

struct BufferBuilder : public Builder<BufferBuilder, vk::BufferCreateInfo>
{
  private:
	using Parent = Builder<BufferBuilder, vk::BufferCreateInfo>;

  public:
	BufferBuilder(vk::DeviceSize size) :
	    Builder(vk::BufferCreateInfo({}, size))
	{
	}

	BufferBuilder &with_usage(vk::BufferUsageFlags usage)
	{
		create_info.usage = usage;
		return *this;
	}

	BufferBuilder &with_flags(vk::BufferCreateFlags flags)
	{
		create_info.flags = flags;
		return *this;
	}

	Buffer build(Device &device) const;
	BufferPtr build_unique(Device &device) const;
};

class Buffer : public Allocated<vk::Buffer>
{
	using Parent = Allocated<vk::Buffer>;

  public:
	static Buffer create_staging_buffer(Device &device, vk::DeviceSize size, const void *data);

	template<typename T>
	static Buffer create_staging_buffer(Device &device, std::vector<T> const &data)
	{
				return create_staging_buffer(device, sizeof(T) * data.size(), data.data());
	}

	Buffer(Device &device, BufferBuilder const &builder);

	Buffer(const Buffer &) = delete;
	Buffer(Buffer &&other) noexcept;
	~Buffer() override;

	Buffer &operator=(const Buffer &) = delete;
	Buffer &operator=(Buffer &&)      = delete;

	uint64_t get_device_address() const;

	vk::DeviceSize get_size() const;

private:
	vk::DeviceSize size_{0};



};

}        // namespace xihe::backend
