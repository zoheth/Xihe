#include "allocated.h"

#include "common/error.h"

namespace xihe::backend
{

VmaAllocator &get_memory_allocator()
{
	static VmaAllocator memory_allocator = VK_NULL_HANDLE;
	return memory_allocator;
}

AllocateBase::AllocateBase(const VmaAllocationCreateInfo &alloc_create_info) :
    alloc_create_info_(alloc_create_info)
{}

AllocateBase::AllocateBase(AllocateBase &&other) noexcept :
    alloc_create_info_(std::exchange(other.alloc_create_info_, {})),
    allocation_(std::exchange(other.allocation_, {})),
    mapped_data_(std::exchange(other.mapped_data_, {})),
    coherent_(std::exchange(other.coherent_, {})),
    persistent_(std::exchange(other.persistent_, {}))
{}

const uint8_t *AllocateBase::get_data() const
{
	return mapped_data_;
}

vk::DeviceMemory AllocateBase::get_memory() const
{
	VmaAllocationInfo alloc_info;
	vmaGetAllocationInfo(get_memory_allocator(), allocation_, &alloc_info);
	return alloc_info.deviceMemory;
}

void AllocateBase::flush(vk::DeviceSize offset, vk::DeviceSize size)
{
	if (!coherent_)
	{
		vmaFlushAllocation(get_memory_allocator(), allocation_, offset, size);
	}
}

bool AllocateBase::mapped() const
{
	return mapped_data_ != nullptr;
}

uint8_t *AllocateBase::map()
{
	if (!persistent_ && !mapped())
	{
		VK_CHECK(vmaMapMemory(get_memory_allocator(), allocation_, reinterpret_cast<void **>(&mapped_data_)));
		assert(mapped_data_);
	}
	return mapped_data_;
}

void AllocateBase::unmap()
{
	if (!persistent_ && mapped())
	{
		vmaUnmapMemory(get_memory_allocator(), allocation_);
		mapped_data_ = nullptr;
	}
}

size_t AllocateBase::update(const uint8_t *data, size_t size, size_t offset)
{
	if (persistent_)
	{
		std::copy_n(data, size, mapped_data_ + offset);
		flush();
	}
	else
	{
		map();
		std::copy_n(data, size, mapped_data_ + offset);
		flush();
		unmap();
	}
	return size;
}

size_t AllocateBase::update(const void *data, size_t size, size_t offset)
{
	return update(reinterpret_cast<const uint8_t *>(data), size, offset);
}

void AllocateBase::post_create(VmaAllocationInfo const &allocation_info)
{
	VkMemoryPropertyFlags memory_properties;
	vmaGetAllocationMemoryProperties(get_memory_allocator(), allocation_, &memory_properties);

	coherent_    = (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	mapped_data_ = static_cast<uint8_t *>(allocation_info.pMappedData);
	persistent_  = mapped();
}

vk::Buffer AllocateBase::create_buffer(vk::BufferCreateInfo const &create_info)
{
	VkBufferCreateInfo const &create_info_c = create_info.operator VkBufferCreateInfo const &();
	VkBuffer                  buffer;

	VmaAllocationInfo allocation_info{};

	VK_CHECK(vmaCreateBuffer(get_memory_allocator(), &create_info_c, &alloc_create_info_, &buffer, &allocation_, &allocation_info));

	post_create(allocation_info);
	return buffer;
}

vk::Image AllocateBase::create_image(vk::ImageCreateInfo const &create_info)
{
	assert(0 < create_info.mipLevels && "Images should have at least one level");
	assert(0 < create_info.arrayLayers && "Images should have at least one layer");
	assert(0 < create_info.usage && "Images should have at least one usage type");

	VkImageCreateInfo const &create_info_c = create_info.operator VkImageCreateInfo const &();
	VkImage                  image;

	VmaAllocationInfo allocation_info{};

	VK_CHECK(vmaCreateImage(get_memory_allocator(), &create_info_c, &alloc_create_info_, &image, &allocation_, &allocation_info));

	post_create(allocation_info);
	return image;
}

void AllocateBase::destroy_buffer(vk::Buffer buffer)
{
	if (buffer != VK_NULL_HANDLE && allocation_ != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyBuffer(get_memory_allocator(), buffer.operator VkBuffer(), allocation_);
		clear();
	}
}

void AllocateBase::destroy_image(vk::Image image)
{
	if (image != VK_NULL_HANDLE && allocation_ != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyImage(get_memory_allocator(), image.operator VkImage(), allocation_);
		clear();
	}
}

void AllocateBase::clear()
{
	mapped_data_       = nullptr;
	persistent_        = false;
	alloc_create_info_ = {};
}
}        // namespace xihe::backend
