#include "pipeline.h"

#include "device.h"

namespace xihe::backend
{
Pipeline::Pipeline(Device &device) :
    device_{device}
{}

Pipeline::Pipeline(Pipeline &&other) noexcept :
    device_{other.device_},
    handle_{other.handle_},
    state_{other.state_}
{
	other.handle_ = VK_NULL_HANDLE;
}

Pipeline::~Pipeline()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		device_.get_handle().destroyPipeline(handle_);
	}
}

VkPipeline Pipeline::get_handle() const
{
	return handle_;
}

const PipelineState &Pipeline::get_state() const
{
	return state_;
}

ComputePipeline::ComputePipeline(Device &device, vk::PipelineCache pipeline_cache, PipelineState &pipeline_state) :
    Pipeline{device}
{
	const ShaderModule *shader_module = pipeline_state.get_pipeline_layout().get_shader_modules().front();

	if (shader_module->get_stage() != vk::ShaderStageFlagBits::eCompute)
	{
		throw VulkanException{vk::Result::eErrorInvalidShaderNV, "Shader module stage is not compute"};
	}

	vk::PipelineShaderStageCreateInfo stage_create_info{
	    vk::PipelineShaderStageCreateFlags{},
	    vk::ShaderStageFlagBits::eCompute,
	    VK_NULL_HANDLE,
	    shader_module->get_entry_point().c_str(),
	    nullptr};

	vk::ShaderModuleCreateInfo shader_module_create_info{
	    vk::ShaderModuleCreateFlags{},
	    shader_module->get_binary().size() * sizeof(uint32_t),
	    reinterpret_cast<const uint32_t *>(shader_module->get_binary().data())};

	vk::Result result = device_.get_handle().createShaderModule(&shader_module_create_info, nullptr, &stage_create_info.module);

	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Failed to create shader module"};
	}

	device.get_debug_utils().set_debug_name(device.get_handle(),
	                                        vk::ObjectType::eShaderModule,
	                                        reinterpret_cast<uint64_t>(static_cast<VkShaderModule>(stage_create_info.module)),
	                                        shader_module->get_debug_name().c_str());

	// Create specialization constant info from tracked state.
	std::vector<uint8_t>                    data{};
	std::vector<vk::SpecializationMapEntry> map_entries{};

	const auto specialization_constant_stat = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

	for (const auto specialization_constant : specialization_constant_stat)
	{
		map_entries.push_back({specialization_constant.first, to_u32(data.size()), specialization_constant.second.size()});
		data.insert(data.end(), specialization_constant.second.begin(), specialization_constant.second.end());
	}

	vk::SpecializationInfo specialization_info{
	    to_u32(map_entries.size()),
	    map_entries.data(),
	    data.size(),
	    data.data()};

	stage_create_info.pSpecializationInfo = &specialization_info;

	vk::ComputePipelineCreateInfo create_info{
	    vk::PipelineCreateFlags{},
	    stage_create_info,
	    pipeline_state.get_pipeline_layout().get_handle(),
	    VK_NULL_HANDLE,
	    -1};

	result = device_.get_handle().createComputePipelines(pipeline_cache, 1, &create_info, nullptr, &handle_);

	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Failed to create compute pipeline"};
	}

	device.get_handle().destroyShaderModule(stage_create_info.module);
}

GraphicsPipeline::GraphicsPipeline(Device &device, vk::PipelineCache pipeline_cache, PipelineState &pipeline_state) :
    Pipeline{device}
{
	std::vector<vk::ShaderModule>                  shader_modules{};
	std::vector<vk::PipelineShaderStageCreateInfo> stage_create_infos{};

	// Create specialization constant info from tracked state. This is shared by all shaders.
	std::vector<uint8_t>                    data{};
	std::vector<vk::SpecializationMapEntry> map_entries{};

	const auto specialization_constant_stat = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

	for (const auto specialization_constant : specialization_constant_stat)
	{
		map_entries.push_back({specialization_constant.first, to_u32(data.size()), specialization_constant.second.size()});
		data.insert(data.end(), specialization_constant.second.begin(), specialization_constant.second.end());
	}

	vk::SpecializationInfo specialization_info{
	    to_u32(map_entries.size()),
	    map_entries.data(),
	    data.size(),
	    data.data()};

	for (const ShaderModule *shader_module : pipeline_state.get_pipeline_layout().get_shader_modules())
	{
		vk::PipelineShaderStageCreateInfo stage_create_info{
		    vk::PipelineShaderStageCreateFlags{},
		    shader_module->get_stage(),
		    VK_NULL_HANDLE,
		    shader_module->get_entry_point().c_str()};

		vk::ShaderModuleCreateInfo shader_module_create_info{
		    vk::ShaderModuleCreateFlags{},
		    shader_module->get_binary().size() * sizeof(uint32_t),
		    shader_module->get_binary().data()};

		vk::Result result = device_.get_handle().createShaderModule(&shader_module_create_info, nullptr, &stage_create_info.module);

		if (result != vk::Result::eSuccess)
		{
			throw VulkanException{result, "Failed to create shader module"};
		}

		device.get_debug_utils().set_debug_name(device.get_handle(),
		                                        vk::ObjectType::eShaderModule,
		                                        reinterpret_cast<uint64_t>(static_cast<VkShaderModule>(stage_create_info.module)),
		                                        shader_module->get_debug_name().c_str());

		stage_create_info.pSpecializationInfo = &specialization_info;

		stage_create_infos.push_back(stage_create_info);
		shader_modules.push_back(stage_create_info.module);
	}

	vk::PipelineViewportStateCreateInfo viewport_state_create_info{
	    vk::PipelineViewportStateCreateFlags{},
	    to_u32(pipeline_state.get_viewport_state().viewport_count),
	    {},
	    to_u32(pipeline_state.get_viewport_state().scissor_count),
	    {}};

	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{
	    vk::PipelineRasterizationStateCreateFlags{},
	    pipeline_state.get_rasterization_state().depth_clamp_enable,
	    pipeline_state.get_rasterization_state().rasterizer_discard_enable,
	    pipeline_state.get_rasterization_state().polygon_mode,
	    pipeline_state.get_rasterization_state().cull_mode,
	    pipeline_state.get_rasterization_state().front_face,
	    pipeline_state.get_rasterization_state().depth_bias_enable,
	    {},
	    1.0f,
	    1.0f,
	    1.0f};

	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{
	    vk::PipelineMultisampleStateCreateFlags{},
	    pipeline_state.get_multisample_state().rasterization_samples,
	    pipeline_state.get_multisample_state().sample_shading_enable,
	    pipeline_state.get_multisample_state().min_sample_shading,
	    {},
	    pipeline_state.get_multisample_state().alpha_to_coverage_enable,
	    pipeline_state.get_multisample_state().alpha_to_one_enable};

	if (pipeline_state.get_multisample_state().sample_mask)
	{
		multisample_state_create_info.pSampleMask = &pipeline_state.get_multisample_state().sample_mask;
	}

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
	    vk::PipelineDepthStencilStateCreateFlags{},
	    pipeline_state.get_depth_stencil_state().depth_test_enable,
	    pipeline_state.get_depth_stencil_state().depth_write_enable,
	    pipeline_state.get_depth_stencil_state().depth_compare_op,
	    pipeline_state.get_depth_stencil_state().depth_bounds_test_enable,
	    pipeline_state.get_depth_stencil_state().stencil_test_enable,
	    pipeline_state.get_depth_stencil_state().front,
	    pipeline_state.get_depth_stencil_state().back};

	std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments{};
	color_blend_attachments.resize(pipeline_state.get_color_blend_state().attachments.size());

	for (size_t i = 0; i < color_blend_attachments.size(); i++)
	{
		const auto &attachment = pipeline_state.get_color_blend_state().attachments[i];

		color_blend_attachments[i] = {
		    attachment.blend_enable,
		    attachment.src_color_blend_factor,
		    attachment.dst_color_blend_factor,
		    attachment.color_blend_op,
		    attachment.src_alpha_blend_factor,
		    attachment.dst_alpha_blend_factor,
		    attachment.alpha_blend_op,
		    attachment.color_write_mask};
	}

	vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{
	    vk::PipelineColorBlendStateCreateFlags{},
	    pipeline_state.get_color_blend_state().logic_op_enable,
	    pipeline_state.get_color_blend_state().logic_op,
	    color_blend_attachments,
	    {1.0f, 1.0f, 1.0f, 1.0f}};

	std::array dynamic_states{
	    vk::DynamicState::eViewport,
	    vk::DynamicState::eScissor,
	    vk::DynamicState::eLineWidth,
	    vk::DynamicState::eDepthBias,
	    vk::DynamicState::eBlendConstants,
	    vk::DynamicState::eDepthBounds,
	    vk::DynamicState::eStencilCompareMask,
	    vk::DynamicState::eStencilWriteMask,
	    vk::DynamicState::eStencilReference};

	vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{
	    vk::PipelineDynamicStateCreateFlags{},
	    to_u32(dynamic_states.size()),
	    dynamic_states.data()};

	vk::GraphicsPipelineCreateInfo create_info{{},
	                                           to_u32(stage_create_infos.size()),
	                                           stage_create_infos.data(),
	                                           nullptr,
	                                           nullptr,
	                                           {},
	                                           &viewport_state_create_info,
	                                           &rasterization_state_create_info,
	                                           &multisample_state_create_info,
	                                           &depth_stencil_state_create_info,
	                                           &color_blend_state_create_info,
	                                           &dynamic_state_create_info,
	                                           pipeline_state.get_pipeline_layout().get_handle(),
	                                           pipeline_state.get_render_pass()->get_handle(),
	                                           pipeline_state.get_subpass_index()};



	if (!pipeline_state.has_mesh_shader())
	{
		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
		    vk::PipelineVertexInputStateCreateFlags{},
		    to_u32(pipeline_state.get_vertex_input_state().bindings.size()),
		    pipeline_state.get_vertex_input_state().bindings.data(),
		    to_u32(pipeline_state.get_vertex_input_state().attributes.size()),
		    pipeline_state.get_vertex_input_state().attributes.data()};

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{
		    vk::PipelineInputAssemblyStateCreateFlags{},
		    pipeline_state.get_input_assembly_state().topology,
		    pipeline_state.get_input_assembly_state().primitive_restart_enable};

		create_info.pVertexInputState = &vertex_input_state_create_info;
		create_info.pInputAssemblyState = &input_assembly_state_create_info;
	}

	vk::Result result = device_.get_handle().createGraphicsPipelines(pipeline_cache, 1, &create_info, nullptr, &handle_);

	if (result != vk::Result::eSuccess)
	{
		throw VulkanException{result, "Failed to create graphics pipeline"};
	}

	for (auto shader_module : shader_modules)
	{
		device.get_handle().destroyShaderModule(shader_module);
	}

	state_ = pipeline_state;
}
}        // namespace xihe::backend
