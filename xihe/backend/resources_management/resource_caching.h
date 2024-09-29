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

template <class T>
void hash_combine(std::size_t &seed, const T &v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

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
struct hash<xihe::common::LoadStoreInfo>
{
	size_t operator()(const xihe::common::LoadStoreInfo &info) const noexcept
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
struct hash<xihe::backend::RenderPass>
{
	std::size_t operator()(const xihe::backend::RenderPass &render_pass) const
	{
		std::size_t result = 0;

		hash_combine(result, render_pass.get_handle());

		return result;
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
	size_t operator()(xihe::backend::DescriptorSet &descriptor_set) const noexcept
	{
		std::size_t result = 0;

		hash_combine(result, descriptor_set.get_layout());
		hash_combine(result, descriptor_set.get_descriptor_pool());
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
struct hash<xihe::SpecializationConstantState>
{
	std::size_t operator()(const xihe::SpecializationConstantState &specialization_constant_state) const
	{
		std::size_t result = 0;

		for (auto constants : specialization_constant_state.get_specialization_constant_state())
		{
			hash_combine(result, constants.first);
			for (const auto data : constants.second)
			{
				hash_combine(result, data);
			}
		}

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
struct hash<xihe::ColorBlendAttachmentState>
{
	std::size_t operator()(const xihe::ColorBlendAttachmentState &color_blend_attachment) const
	{
		std::size_t result = 0;

		hash_combine(result, color_blend_attachment.alpha_blend_op);
		hash_combine(result, color_blend_attachment.blend_enable);
		hash_combine(result, color_blend_attachment.color_blend_op);
		hash_combine(result, color_blend_attachment.color_write_mask);
		hash_combine(result, color_blend_attachment.dst_alpha_blend_factor);
		hash_combine(result, color_blend_attachment.dst_color_blend_factor);
		hash_combine(result, color_blend_attachment.src_alpha_blend_factor);
		hash_combine(result, color_blend_attachment.src_color_blend_factor);

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
			hash_combine(result, view.get_handle());
			hash_combine(result, view.get_image().get_handle());
		}
		/*for (auto const &attachment : render_target.get_attachments())
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
		}*/
		return result;
	}
};

}        // namespace std

namespace xihe::backend
{
namespace
{

template <typename T>
inline void hash_param(size_t &seed, const T &value)
{
	hash_combine(seed, value);
}

template <>
inline void hash_param(size_t & /*seed*/, const vk::PipelineCache & /*value*/)
{
}

template <>
inline void hash_param<std::vector<uint8_t>>(
    size_t                     &seed,
    const std::vector<uint8_t> &value)
{
	hash_combine(seed, std::string{value.begin(), value.end()});
}

template <>
inline void hash_param<std::vector<xihe::rendering::Attachment>>(
    size_t                        &seed,
    const std::vector<xihe::rendering::Attachment> &value)
{
	for (auto &attachment : value)
	{
		hash_combine(seed, attachment);
	}
}

template <>
inline void hash_param<std::vector<common::LoadStoreInfo>>(
    size_t                           &        seed,
    const std::vector<common::LoadStoreInfo> &value)
{
	for (auto &load_store_info : value)
	{
		hash_combine(seed, load_store_info);
	}
}

template <>
inline void hash_param<std::vector<SubpassInfo>>(
    size_t                         &seed,
    const std::vector<SubpassInfo> &value)
{
	for (auto &subpass_info : value)
	{
		hash_combine(seed, subpass_info);
	}
}

template <>
inline void hash_param<std::vector<ShaderModule *>>(
    size_t                            &seed,
    const std::vector<ShaderModule *> &value)
{
	for (auto &shader_module : value)
	{
		hash_combine(seed, shader_module->get_id());
	}
}

template <>
inline void hash_param<std::vector<ShaderResource>>(
    size_t                            &seed,
    const std::vector<ShaderResource> &value)
{
	for (auto &resource : value)
	{
		hash_combine(seed, resource);
	}
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, vk::DescriptorBufferInfo>>>(
    size_t                                                               &seed,
    const std::map<uint32_t, std::map<uint32_t, vk::DescriptorBufferInfo>> &value)
{
	for (auto &binding_set : value)
	{
		hash_combine(seed, binding_set.first);

		for (auto &binding_element : binding_set.second)
		{
			hash_combine(seed, binding_element.first);
			hash_combine(seed, binding_element.second);
		}
	}
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, vk::DescriptorImageInfo>>>(
    size_t                                                              &seed,
    const std::map<uint32_t, std::map<uint32_t, vk::DescriptorImageInfo>> &value)
{
	for (auto &binding_set : value)
	{
		hash_combine(seed, binding_set.first);

		for (auto &binding_element : binding_set.second)
		{
			hash_combine(seed, binding_element.first);
			hash_combine(seed, binding_element.second);
		}
	}
}

template <typename T, typename... Args>
inline void hash_param(size_t &seed, const T &first_arg, const Args &...args)
{
	hash_param(seed, first_arg);

	hash_param(seed, args...);
}

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

template <class... A>
struct RecordHelper<ShaderModule, A...>
{
	size_t record(ResourceRecord &recorder, A &...args)
	{
		return recorder.register_shader_module(args...);
	}

	void index(ResourceRecord &recorder, size_t index, ShaderModule &shader_module)
	{
		recorder.set_shader_module(index, shader_module);
	}
};

template <class... A>
struct RecordHelper<PipelineLayout, A...>
{
	size_t record(ResourceRecord &recorder, A &...args)
	{
		return recorder.register_pipeline_layout(args...);
	}

	void index(ResourceRecord &recorder, size_t index, PipelineLayout &pipeline_layout)
	{
		recorder.set_pipeline_layout(index, pipeline_layout);
	}
};

template <class... A>
struct RecordHelper<RenderPass, A...>
{
	size_t record(ResourceRecord &recorder, A &...args)
	{
		return recorder.register_render_pass(args...);
	}

	void index(ResourceRecord &recorder, size_t index, RenderPass &render_pass)
	{
		recorder.set_render_pass(index, render_pass);
	}
};

template <class... A>
struct RecordHelper<GraphicsPipeline, A...>
{
	size_t record(ResourceRecord &recorder, A &...args)
	{
		return recorder.register_graphics_pipeline(args...);
	}

	void index(ResourceRecord &recorder, size_t index, GraphicsPipeline &graphics_pipeline)
	{
		recorder.set_graphics_pipeline(index, graphics_pipeline);
	}
};
}        // namespace

template <class T, class... A>
T &request_resource(Device &device, ResourceRecord *recorder, std::unordered_map<size_t, T> &resources, A &...args)
{
	RecordHelper<T, A...> record_helper;

	std::size_t hash{0U};
	hash_param(hash, args...);

	auto res_it = resources.find(hash);

	if (res_it != resources.end())
	{
		return res_it->second;
	}

	// If we do not have it already, create and cache it
	const char *res_type = typeid(T).name();
	size_t      res_id   = resources.size();

	LOGD("Building #{} cache object ({})", res_id, res_type);
	hash = 0;
	hash_param(hash, args...);

	// Only error handle in release
#ifndef XH_DEBUG
	try
	{
#endif
		T resource(device, args...);

		auto res_ins_it = resources.emplace(hash, std::move(resource));

		if (!res_ins_it.second)
		{
			throw std::runtime_error{std::string{"Insertion error for #"} + std::to_string(res_id) + "cache object (" + res_type + ")"};
		}

		res_it = res_ins_it.first;

		if (recorder)
		{
			size_t index = record_helper.record(*recorder, args...);
			record_helper.index(*recorder, index, res_it->second);
		}
#ifndef XH_DEBUG
	}
	catch (const std::exception &e)
	{
		LOGE("Creation error for #{} cache object ({})", res_id, res_type);
		throw e;
	}
#endif

	return res_it->second;
}

}        // namespace xihe::backend
