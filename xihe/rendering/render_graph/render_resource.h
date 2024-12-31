#pragma once

#include "backend/buffer.h"
#include "backend/image_view.h"
#include "vulkan/vulkan.hpp"

#include <variant>

namespace xihe::rendering
{
// typedef std::string ResourceHandle;

struct ResourceHandle
{
	std::string name;
	uint32_t    base_layer  = 0;
	uint32_t    layer_count = 0;

	bool operator==(const ResourceHandle &) const = default;
};

class ShaderBindable
{
  public:
	ShaderBindable() = default;

	ShaderBindable(backend::Buffer *buffer) :
	    resource_{buffer}
	{}

	ShaderBindable(backend::ImageView *image_view) :
	    resource_{image_view}
	{}

	ShaderBindable &operator=(std::variant<backend::Buffer *, backend::ImageView *> resource)
	{
		resource_ = resource;
		return *this;
	}

	bool is_buffer() const
	{
		return std::holds_alternative<backend::Buffer *>(resource_);
	}

	bool is_image_view() const
	{
		return std::holds_alternative<backend::ImageView *>(resource_);
	}

	backend::ImageView &image_view() const
	{
		if (auto *view = std::get_if<backend::ImageView *>(&resource_))
		{
			if (*view)
			{
				return **view;
			}
			throw std::runtime_error("Image view is null");
		}
		throw std::runtime_error("Resource is not an image view");
	}

	backend::Buffer &buffer() const
	{
		if (auto *buf = std::get_if<backend::Buffer *>(&resource_))
		{
			if (*buf)
			{
				return **buf;
			}
			throw std::runtime_error("Buffer is null");
		}
		throw std::runtime_error("Resource is not a buffer");
	}

	template <typename Visitor>
	auto visit(Visitor &&visitor) const
	{
		return std::visit(std::forward<Visitor>(visitor), resource_);
	}

  private:
	std::variant<backend::Buffer *, backend::ImageView *> resource_{};
};

struct ResourceInfo
{
	bool external = false;

	struct BufferDesc
	{
		vk::BufferUsageFlags usage;
		uint32_t             size;
		backend::Buffer     *buffer = nullptr;
	};

	struct ImageDesc
	{
		vk::Format          format = vk::Format::eUndefined;
		vk::Extent3D        extent{};
		vk::ImageUsageFlags usage;
		backend::ImageView *image_view = nullptr;
	};

	std::variant<BufferDesc, ImageDesc> desc;

	ShaderBindable get_bindable() const
	{
		return std::visit([]<typename Desc>(const Desc &resource) -> ShaderBindable {
			if constexpr (std::is_same_v<std::decay_t<Desc>, BufferDesc>)
			{
				return ShaderBindable(resource.buffer);
			}
			else
			{
				return ShaderBindable(resource.image_view);
			}
		},
		                  desc);
	}
};

enum class PassType
{
	kNone    = 0,
	kRaster  = 1 << 0,
	kMesh    = 1 << 1,
	kCompute = 1 << 2,
};

enum class BindableType
{
	kSampled,                      // uniform sampler2D
	kStorageRead,                  // layout(rgba8) readonly uniform image2D
	kStorageWrite,                 // layout(rgba8) writeonly uniform image2D
	kStorageReadWrite,             // layout(rgba8) uniform image2D
	kUniformBuffer,                // uniform buffer
	kStorageBufferRead,            // layout(std430) readonly buffer
	kIndirectBuffer,                // indirect buffer
	kStorageBufferWrite,           // layout(std430) writeonly buffer
	kStorageBufferWriteClear,
	kStorageBufferReadWrite,        // layout(std430) buffer
};

enum class AttachmentType
{
	kColor,               // layout(location = X) out vec4
	kDepth,               // depth attachment
	kDepthStencil,        // depth stencil attachment
	kReference            // reference attachment: This attachment is created elsewhere, such as being part of an array
};

struct ResourceUsageState
{
	vk::ImageLayout         layout{vk::ImageLayout::eUndefined};
	vk::PipelineStageFlags2 stage_mask{};
	vk::AccessFlags2        access_mask{};
};

bool is_buffer(BindableType type);

void update_bindable_state(BindableType type, PassType pass_type, ResourceUsageState &state);

void update_attachment_state(AttachmentType type, ResourceUsageState &state);

}        // namespace xihe::rendering

template <>
struct std::hash<xihe::rendering::ResourceHandle>
{
	size_t operator()(const xihe::rendering::ResourceHandle &handle) const
	{
		return std::hash<std::string>{}(handle.name) ^
		       std::hash<uint32_t>{}(handle.base_layer) ^
		       std::hash<uint32_t>{}(handle.layer_count);
	}
};
