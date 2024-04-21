#pragma once

#include <vulkan/vulkan_hash.hpp>

#include "backend/descriptor_pool.h"
#include "backend/descriptor_set.h"
#include "backend/descriptor_set_layout.h"
#include "backend/device.h"
#include "backend/framebuffer.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"
#include "resource_record.h"

namespace
{
template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
}        // namespace

namespace std
{
template <typename Key, typename Value>
struct hash<std::map<Key, Value>>
{
	size_t operator()(const std::map<Key, Value> &map) const
	{
		size_t result = 0;
		hash_combine(result, map.size());
		for (const auto &pair : map)
		{
			hash_combine(result, pair.first);
			hash_combine(result, pair.second);
		}
		return result;
	}
};

template <typename T>
struct hash<std::vector<T>>
{
	size_t operator()(const std::vector<T> &vec) const
	{
		size_t result = 0;
		hash_combine(result, vec.size());
		for (const auto &value : vec)
		{
			hash_combine(result, value);
		}
		return result;
	}
};

template <>
struct hash<xihe::LoadStoreInfo>
{
	size_t operator()(const xihe::LoadStoreInfo &info) const noexcept
	{
		size_t result = 0;
		hash_combine(result, info.load_op);
		hash_combine(result, info.store_op);
		return result;
	}
};

template <typename T>
struct hash<xihe::backend::VulkanResource<T>>
{
	size_t operator()(const xihe::backend::VulkanResource<T> &resource) const noexcept
	{
		return std::hash<T>()(resource.get_handle());
	}
};

template <>
struct hash<xihe::backend::DescriptorSetLayout>
{
	size_t operator()(const xihe::backend::DescriptorSetLayout &descriptor_set_layout) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, descriptor_set_layout.get_handle());

		return result;
	}
};

template <>
struct hash<xihe::backend::DescriptorPool>
{
	size_t operator()(const xihe::backend::DescriptorPool &descriptor_pool) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, descriptor_pool.get_descriptor_set_layout());

		return result;
	}
};

template <>
struct hash<xihe::backend::DescriptorSet>
{
	size_t operator()(const xihe::backend::DescriptorSet &descriptor_set) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, descriptor_set.get_layout());
		// descriptor_pool ?
		hash_combine(result, descriptor_set.get_buffer_infos());
		hash_combine(result, descriptor_set.get_image_infos());
		hash_combine(result, descriptor_set.get_handle());
		// write_descriptor_sets ?

		return result;
	}
};

template <>
struct hash<xihe::backend::PipelineLayout>
{
	size_t operator()(const xihe::backend::PipelineLayout &descriptor_set) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, descriptor_set.get_handle());

		return result;
	}
};

template <>
struct hash<xihe::backend::Image>
{
	size_t operator()(const xihe::backend::Image &image) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, image.get_memory());
		hash_combine(result, image.get_type());
		hash_combine(result, image.get_extent());
		hash_combine(result, image.get_format());
		hash_combine(result, image.get_usage());
		hash_combine(result, image.get_sample_count());
		hash_combine(result, image.get_tiling());
		hash_combine(result, image.get_subresource());
		hash_combine(result, image.get_array_layer_count());

		return result;
	}
};

template <>
struct hash<xihe::backend::ImageView>
{
	size_t operator()(const xihe::backend::ImageView &image_view) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, image_view.get_image());
		hash_combine(result, image_view.get_format());
		hash_combine(result, image_view.get_subresource_range());

		return result;
	}
};

// RenderPass is VulkanResource

template <>
struct hash<xihe::backend::ShaderModule>
{
	size_t operator()(const xihe::backend::ShaderModule &shader_module) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, shader_module.get_id());

		return result;
	}
};

template <>
struct hash<xihe::backend::ShaderResource>
{
	size_t operator()(const xihe::backend::ShaderResource &shader_resource) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, shader_resource.stages);
		hash_combine(result, shader_resource.type);
		hash_combine(result, shader_resource.mode);
		hash_combine(result, shader_resource.set);
		hash_combine(result, shader_resource.binding);
		hash_combine(result, shader_resource.location);
		hash_combine(result, shader_resource.input_attachment_index);
		hash_combine(result, shader_resource.vec_size);
		hash_combine(result, shader_resource.columns);
		hash_combine(result, shader_resource.array_size);
		hash_combine(result, shader_resource.offset);
		hash_combine(result, shader_resource.size);
		hash_combine(result, shader_resource.constant_id);
		hash_combine(result, shader_resource.qualifiers);
		hash_combine(result, shader_resource.name);

		return result;
	}
};

template <>
struct hash<xihe::backend::ShaderSource>
{
	size_t operator()(const xihe::backend::ShaderSource &shader_source) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, shader_source.get_id());

		return result;
	}
};

template <>
struct hash<xihe::backend::ShaderVariant>
{
	size_t operator()(const xihe::backend::ShaderVariant &shader_variant) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, shader_variant.get_id());

		return result;
	}
};

template <>
struct hash<xihe::backend::SubpassInfo>
{
	size_t operator()(const xihe::backend::SubpassInfo &subpass_info) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, subpass_info.input_attachments);
		hash_combine(result, subpass_info.output_attachments);
		hash_combine(result, subpass_info.color_resolve_attachments);
		hash_combine(result, subpass_info.disable_depth_stencil_attachment);
		hash_combine(result, subpass_info.depth_stencil_resolve_attachment);
		hash_combine(result, subpass_info.depth_stencil_resolve_mode);
		hash_combine(result, subpass_info.debug_name);

		return result;
	}
};

template <>
struct hash<xihe::rendering::Attachment>
{
	std::size_t operator()(const xihe::rendering::Attachment &attachment) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, attachment.format);
		hash_combine(result, attachment.samples);
		hash_combine(result, attachment.usage);
		hash_combine(result, attachment.initial_layout);

		return result;
	}
};

template <>
struct hash<xihe::PipelineState>
{
	std::size_t operator()(const xihe::PipelineState &pipeline_state) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, pipeline_state.get_pipeline_layout().get_handle());

		// For graphics only
		if (auto render_pass = pipeline_state.get_render_pass())
		{
			hash_combine(result, render_pass->get_handle());
		}

		hash_combine(result, pipeline_state.get_specialization_constant_state());

		hash_combine(result, pipeline_state.get_subpass_index());

		for (auto shader_module : pipeline_state.get_pipeline_layout().get_shader_modules())
		{
			hash_combine(result, shader_module->get_id());
		}

		// VkPipelineVertexInputStateCreateInfo
		for (auto &attribute : pipeline_state.get_vertex_input_state().attributes)
		{
			hash_combine(result, attribute);
		}

		for (auto &binding : pipeline_state.get_vertex_input_state().bindings)
		{
			hash_combine(result, binding);
		}

		// VkPipelineInputAssemblyStateCreateInfo
		hash_combine(result, pipeline_state.get_input_assembly_state().primitive_restart_enable);
		hash_combine(result, pipeline_state.get_input_assembly_state().topology);

		// VkPipelineViewportStateCreateInfo
		hash_combine(result, pipeline_state.get_viewport_state().viewport_count);
		hash_combine(result, pipeline_state.get_viewport_state().scissor_count);

		// VkPipelineRasterizationStateCreateInfo
		hash_combine(result, pipeline_state.get_rasterization_state().cull_mode);
		hash_combine(result, pipeline_state.get_rasterization_state().depth_bias_enable);
		hash_combine(result, pipeline_state.get_rasterization_state().depth_clamp_enable);
		hash_combine(result, pipeline_state.get_rasterization_state().front_face);
		hash_combine(result, pipeline_state.get_rasterization_state().polygon_mode);
		hash_combine(result, pipeline_state.get_rasterization_state().rasterizer_discard_enable);

		// VkPipelineMultisampleStateCreateInfo
		hash_combine(result, pipeline_state.get_multisample_state().alpha_to_coverage_enable);
		hash_combine(result, pipeline_state.get_multisample_state().alpha_to_one_enable);
		hash_combine(result, pipeline_state.get_multisample_state().min_sample_shading);
		hash_combine(result, pipeline_state.get_multisample_state().rasterization_samples);
		hash_combine(result, pipeline_state.get_multisample_state().sample_shading_enable);
		hash_combine(result, pipeline_state.get_multisample_state().sample_mask);

		// VkPipelineDepthStencilStateCreateInfo
		hash_combine(result, pipeline_state.get_depth_stencil_state().back);
		hash_combine(result, pipeline_state.get_depth_stencil_state().depth_bounds_test_enable);
		hash_combine(result, pipeline_state.get_depth_stencil_state().depth_compare_op);
		hash_combine(result, pipeline_state.get_depth_stencil_state().depth_test_enable);
		hash_combine(result, pipeline_state.get_depth_stencil_state().depth_write_enable);
		hash_combine(result, pipeline_state.get_depth_stencil_state().front);
		hash_combine(result, pipeline_state.get_depth_stencil_state().stencil_test_enable);

		// VkPipelineColorBlendStateCreateInfo
		hash_combine(result, pipeline_state.get_color_blend_state().logic_op);
		hash_combine(result, pipeline_state.get_color_blend_state().logic_op_enable);

		for (auto &attachment : pipeline_state.get_color_blend_state().attachments)
		{
			hash_combine(result, attachment);
		}

		return result;
	}
};

template <>
struct hash<xihe::rendering::RenderTarget>
{
	std::size_t operator()(const xihe::rendering::RenderTarget &render_target) const noexcept
	{
		std::size_t result = 0;
		hash_combine(result, render_target.get_extent());

		for (auto const &view : render_target.get_views())
		{
			hash_combine(result, view);
		}
		for (auto const &attachment : render_target.get_attachments())
		{
			hash_combine(result, attachment);
		}
		for (auto const &input : render_target.get_input_attachments())
		{
			hash_combine(result, input);
		}
		for (auto const &output : render_target.get_output_attachments())
		{
			hash_combine(result, output);
		}
		return result;
	}
};

}        // namespace std

namespace xihe::backend
{
namespace
{
template <class T, class... A>
struct RecordHelper
{
	size_t record(ResourceRecord &, A &...)
	{
		return 0;
	}

	void index(ResourceRecord &, size_t, T &)
	{
	}
};
}        // namespace

template <class T, class... A>
T &request_resource(Device &device, ResourceRecord *recorder, std::unordered_map<size_t, T> &resources, A &&...args)
{
}

}        // namespace xihe::backend
