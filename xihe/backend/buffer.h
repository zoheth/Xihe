#pragma once
#include <memory>

#include "backend/allocated.h"
#include "backend/vulkan_resource.h"

namespace xihe::backend
{
class Device;
class Buffer;
using BufferPtr = std::unique_ptr<Buffer>;

struct BufferBuilder : public allocated::Builder<BufferBuilder, vk::BufferCreateInfo>
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

	Buffer    build(Device &device) const;
	BufferPtr build_unique(Device &device) const;
};

class Buffer : public allocated::Allocated<vk::Buffer>
{
	using Parent = Allocated<vk::Buffer>;

  public:
	static Buffer create_staging_buffer(Device &device, vk::DeviceSize size, const void *data);

	template <typename T>
	static Buffer create_staging_buffer(Device &device, std::vector<T> const &data)
	{
		return create_staging_buffer(device, sizeof(T) * data.size(), data.data());
	}

	template <typename T>
	static Buffer create_staging_buffer(Device &device, const T &data)
	{
		return create_staging_buffer(device, sizeof(T), &data);
	}

	Buffer(Device &device, BufferBuilder const &builder);

	Buffer(const Buffer &) = delete;
	Buffer(Buffer &&other) noexcept;
	~Buffer() override;

	Buffer &operator=(const Buffer &) = delete;
	Buffer &operator=(Buffer &&)      = delete;

	// FIXME should include a stride parameter, because if you want to copy out of a
	// uniform buffer that's dynamically indexed, you need to account for the aligned
	// offset of the T values
	template <typename T>
	static std::vector<T> copy(std::unordered_map<std::string, backend::Buffer> &buffers, const char *buffer_name)
	{
		auto iter = buffers.find(buffer_name);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto          &buffer = iter->second;
		std::vector<T> out;

		const size_t sz = buffer.get_size();
		out.resize(sz / sizeof(T));
		const bool already_mapped = buffer.get_data() != nullptr;
		if (!already_mapped)
		{
			buffer.map();
		}
		memcpy(&out[0], buffer.get_data(), sz);
		if (!already_mapped)
		{
			buffer.unmap();
		}
		return out;
	}

	uint64_t get_device_address() const;

	vk::DeviceSize get_size() const;

  private:
	vk::DeviceSize size_{0};
};

}        // namespace xihe::backend
