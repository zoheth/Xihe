#include "buffer.h"

#include "backend/device.h"

namespace xihe::backend
{
Buffer BufferBuilder::build(Device &device) const
{
	return Buffer(device, *this);
}

BufferPtr BufferBuilder::build_unique(Device &device) const
{
	return std::make_unique<Buffer>(device, *this);
}

Buffer Buffer::create_staging_buffer(Device &device, vk::DeviceSize size, const void *data)
{
	BufferBuilder builder{size};
	builder.with_usage(vk::BufferUsageFlagBits::eTransferSrc);
	builder.with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
	auto staging_buffer = builder.build(device);

	if (data != nullptr)
	{
		staging_buffer.update(static_cast<const uint8_t *>(data), size);
	}
	return staging_buffer;
}

Buffer::Buffer(Device &device, BufferBuilder const &builder) :
Parent{builder.allocation_create_info, nullptr, &device},
size_{builder.create_info.size}
{
	get_handle() = create_buffer(builder.create_info);

	if (!builder.debug_name.empty())
	{
		set_debug_name(builder.debug_name);
	}
}

Buffer::Buffer(Buffer &&other) noexcept:
    Allocated{static_cast<Allocated &&>(other)},
    size_(std::exchange(other.size_, {}))
{}

Buffer::~Buffer()
{
	destroy_buffer(get_handle());
}

uint64_t Buffer::get_device_address() const
{
	return get_device().get_handle().getBufferAddressKHR({get_handle()});
}

vk::DeviceSize Buffer::get_size() const
{
	return size_;
}
}
