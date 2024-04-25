#pragma once

#include <string>
#include <utility>

#include <vulkan/vulkan.hpp>

namespace xihe::backend
{
class Device;

template <typename Handle>
class VulkanResource
{
  public:
	VulkanResource(Handle handle = nullptr, Device *device = nullptr) :
	    handle_(handle), device_(device)
	{}

	VulkanResource(const VulkanResource &)            = delete;
	VulkanResource &operator=(const VulkanResource &) = delete;

	VulkanResource(VulkanResource &&other) noexcept :
	    handle_(std::exchange(other.handle_, {})), device_(std::exchange(other.device_, {}))
	{
		set_debug_name(std::exchange(other.debug_name_, {}));
	}

	VulkanResource &operator=(VulkanResource &&other) noexcept
	{
		if (this != &other)
		{
			handle_     = std::exchange(other.handle_, {});
			device_     = std::exchange(other.device_, {});
			set_debug_name(std::exchange(other.debug_name_, {}));
		}
		return *this;
	}

	virtual ~VulkanResource() = default;

	vk::ObjectType get_object_type() const
	{
		return Handle::NativeType;
	}

	Handle &get_handle()
	{
		return handle_;
	}
	Handle get_handle() const
	{
		return handle_;
	}
	Device &get_device()
	{
		assert(device_ && "VKBDevice handle not set");
		return *device_;
	}
	Device const &get_device() const
	{
		assert(device_ && "VKBDevice handle not set");
		return *device_;
	}

	uint64_t get_handle_u64() const
	{
		// See https://github.com/KhronosGroup/Vulkan-Docs/issues/368 .
		// Dispatchable and non-dispatchable handle types are *not* necessarily binary-compatible!
		// Non-dispatchable handles _might_ be only 32-bit long. This is because, on 32-bit machines, they might be a typedef to a 32-bit pointer.
		using UintHandle = typename std::conditional<sizeof(Handle) == sizeof(uint32_t), uint32_t, uint64_t>::type;

		return static_cast<uint64_t>(*reinterpret_cast<UintHandle const *>(&handle_));
	}

	void set_handle(Handle hdl)
	{
		handle_ = hdl;
	}

	const std::string &get_debug_name() const
	{
		return debug_name_;
	}

	void set_debug_name(const std::string &name)
	{
		debug_name_ = name;

		if (device_ && !debug_name_.empty())
		{
			device_->get_debug_utils().set_debug_name(device_->get_handle(), Handle::objectType, get_handle_u64(), debug_name_.c_str());
		}
	}

  private:
	Handle      handle_;
	Device     *device_;
	std::string debug_name_;
};
}        // namespace xihe::backend
