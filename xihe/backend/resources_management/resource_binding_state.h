#pragma once

#include "common/vk_common.h"
#include "backend/buffer.h"
#include "backend/image_view.h"
#include "backend/sampler.h"

namespace xihe::backend
{
struct ResourceInfo
{
	bool                   dirty{false};
	const backend::Buffer *buffer{nullptr};

	vk::DeviceSize offset{0};

	vk::DeviceSize range{0};

	const backend::ImageView *image_view{nullptr};

	const backend::Sampler *sampler{nullptr};
};

class ResourceSet
{
  public:
	void reset();

	bool is_dirty() const;

	void clear_dirty();
	void clear_dirty(uint32_t binding, uint32_t array_element);

	void bind_buffer(const Buffer &buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t binding, uint32_t array_element);
	void bind_image(const ImageView &image_view, const Sampler &sampler, uint32_t binding, uint32_t array_element);

	void bind_image(const ImageView &image_view, uint32_t binding, uint32_t array_element);
	void bind_input(const ImageView &image_view, uint32_t binding, uint32_t array_element);

	const BindingMap<ResourceInfo> &get_resource_bindings() const;

  private:
	bool dirty_{false};

	BindingMap<ResourceInfo> resource_bindings_;
};

class ResourceBindingState
{
  public:
	void reset();

	bool is_dirty() const;

	void clear_dirty();

	void clear_dirty(uint32_t set);

	void bind_buffer(const Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const ImageView &image_view, const Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_input(const ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	const std::unordered_map<uint32_t, ResourceSet> &get_resource_sets();

  private:
	bool dirty_{false};

	std::unordered_map<uint32_t, ResourceSet> resource_sets_;
};
}        // namespace xihe::backend
