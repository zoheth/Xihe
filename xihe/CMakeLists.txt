project(xihe LANGUAGES C CXX)

option(USE_VULKAN_DEBUG "Use Vulkan debug utilities" ON)

if(USE_VULKAN_DEBUG)
  add_compile_definitions(XH_VULKAN_DEBUG)
endif()

# mask out the min/max macros from minwindef.h
if(MSVC)
    add_definitions(-DNOMINMAX)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})


add_executable (xihe WIN32 "xihe_app.cpp" "xihe_app.h" "backend/instance.h" "backend/instance.cpp" "platform/window.h" "platform/window.cpp" "common/logging.h" "common/error.h" "common/error.cpp" "common/strings.h" "common/strings.cpp" "platform/glfw_window.h" "platform/glfw_window.cpp" "backend/debug.h" "backend/debug.cpp" "backend/physical_device.h" "backend/physical_device.cpp" "backend/device.h" "backend/device.cpp" "backend/vulkan_resource.h" "backend/resources_management/resource_cache.h" "backend/resources_management/resource_cache.cpp" "backend/queue.h" "backend/queue.cpp" "backend/command_pool.h" "backend/command_pool.cpp" "backend/command_buffer.h" "backend/command_buffer.cpp" "backend/fence_pool.h" "backend/fence_pool.cpp" "rendering/render_context.h" "rendering/render_context.cpp" "backend/swapchain.h" "backend/swapchain.cpp" "rendering/render_target.h" "rendering/render_target.cpp" "backend/image.h" "backend/image.cpp" "rendering/render_frame.h" "rendering/render_frame.cpp" "backend/descriptor_pool.h" "backend/descriptor_pool.cpp" "backend/descriptor_set_layout.h" "backend/descriptor_set_layout.cpp" "backend/buffer_pool.h" "backend/buffer_pool.cpp" "backend/descriptor_set.h" "backend/descriptor_set.cpp" "backend/semaphore_pool.h" "backend/semaphore_pool.cpp" "main.cpp" "platform/platform.h" "platform/platform.cpp" "platform/windows/windows_platform.h" "platform/windows/windows_platform.cpp" "platform/input_events.h" "platform/application.h" "platform/application.cpp" "common/timer.h" "common/timer.cpp" "common/vk_common.h" "common/vk_common.cpp" "backend/image_view.h" "backend/image_view.cpp" "platform/input_events.cpp" "backend/shader_module.h" "backend/shader_module.cpp" "platform/filesystem.h" "platform/filesystem.cpp" "backend/shader_compiler/glsl_compiler.h" "backend/shader_compiler/glsl_compiler.cpp" "backend/shader_compiler/spirv_reflection.h" "backend/shader_compiler/spirv_reflection.cpp" "common/helpers.h" "backend/pipeline_layout.h" "backend/pipeline_layout.cpp" "backend/pipeline.h" "backend/pipeline.cpp" "rendering/pipeline_state.h" "rendering/pipeline_state.cpp" "backend/resources_management/resource_record.h" "backend/resources_management/resource_record.cpp" "backend/resources_management/resource_caching.h" "common/glm_common.h" "backend/resources_management/resource_binding_state.h" "backend/resources_management/resource_binding_state.cpp" "backend/buffer.h" "backend/buffer.cpp" "backend/allocated.h" "backend/allocated.cpp" "backend/sampler.h" "backend/sampler.cpp" "rendering/render_pipeline.h" "rendering/render_pipeline.cpp" "test_app.h" "test_app.cpp" "rendering/subpass.h" "rendering/subpass.cpp" "scene_graph/scene.h" "scene_graph/scene.cpp" "scene_graph/gltf_loader.h" "scene_graph/gltf_loader.cpp" "scene_graph/component.h" "scene_graph/component.cpp" "scene_graph/node.h" "scene_graph/node.cpp" "scene_graph/script.h" "scene_graph/script.cpp" "scene_graph/components/transform.h" "scene_graph/components/transform.cpp" "scene_graph/components/material.h" "scene_graph/components/material.cpp" "scene_graph/components/light.h" "scene_graph/components/light.cpp" "scene_graph/components/image.h" "scene_graph/components/image.cpp" "scene_graph/components/image/stb.h" "scene_graph/components/image/stb.cpp" "scene_graph/components/image/astc.h" "scene_graph/components/image/astc.cpp" "scene_graph/components/image/ktx.h" "scene_graph/components/image/ktx.cpp" "scene_graph/components/texture.h" "scene_graph/components/texture.cpp" "scene_graph/components/sampler.h" "scene_graph/components/sampler.cpp" "scene_graph/components/sub_mesh.h" "scene_graph/components/sub_mesh.cpp" "scene_graph/components/camera.h" "scene_graph/components/camera.cpp" "scene_graph/components/mesh.h" "scene_graph/components/mesh.cpp" "scene_graph/components/aabb.h" "scene_graph/components/aabb.cpp" "rendering/subpasses/geometry_subpass.h" "rendering/subpasses/geometry_subpass.cpp" "scene_graph/scripts/free_camera.h" "scene_graph/scripts/free_camera.cpp" "rendering/subpasses/forward_subpass.h" "rendering/subpasses/forward_subpass.cpp" "rendering/subpasses/lighting_subpass.h" "rendering/subpasses/lighting_subpass.cpp" "rendering/subpasses/shadow_subpass.h" "rendering/subpasses/shadow_subpass.cpp" "rendering/subpasses/pure_subpass.h" "rendering/subpasses/pure_subpass.cpp" "rendering/rdg_pass.h" "rendering/rdg_pass.cpp" "rendering/rdg_builder.h" "rendering/rdg_builder.cpp" "rendering/rdg.h" "rendering/rdg.cpp" "scene_graph/scripts/cascade_script.h" "scene_graph/scripts/cascade_script.cpp" "rendering/subpasses/composite_subpass.h" "rendering/subpasses/composite_subpass.cpp" "rendering/post_processing.h" "rendering/post_processing.cpp" "mesh_shading_app.h" "mesh_shading_app.cpp" "rendering/subpasses/mesh_shading_test_subpass.h" "rendering/subpasses/mesh_shading_test_subpass.cpp" "rendering/subpasses/gpu_scene_subpass.h" "rendering/subpasses/gpu_scene_subpass.cpp" "rendering/subpasses/shared_uniform.h" "scene_graph/geometry_data.h" "scene_graph/gpu_scene.h" "scene_graph/gpu_scene.cpp" "rendering/subpasses/meshlet_subpass.h" "rendering/subpasses/meshlet_subpass.cpp" "scene_graph/components/mshader_mesh.h" "scene_graph/components/mshader_mesh.cpp" "gui.h" "gui.cpp" "stats/stats.h" "stats/stats.cpp" "stats/stats_provider.h" "stats/stats_provider.cpp" "stats/stats_common.h" "stats/frame_time_provider.h" "temp_app.h" "temp_app.cpp" "rendering/passes/geometry_pass.h" "rendering/render_pass.h" "rendering/render_graph/render_resource.h" "rendering/render_graph/render_graph.h" "rendering/render_graph/graph_builder.h" "rendering/render_graph/graph_builder.cpp" "rendering/passes/geometry_pass.cpp" "rendering/render_graph/render_graph.cpp" "rendering/passes/render_pass.h" "rendering/passes/render_pass.cpp")

#if (CMAKE_VERSION VERSION_GREATER 3.12)
set_property(TARGET xihe PROPERTY CXX_STANDARD 20)
#endif()

# target_compile_definitions(xihe PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Link third party libraries
target_link_libraries(xihe PUBLIC
    volk
    glm
    spdlog
    ktx
    stb
     ${ASTC_TARGET}
    ctpl
    enkits
    glfw
    tinygltf
    vma
    glslang
    SPIRV
    spirv-cross-glsl
    glslang-default-resource-limits
    meshoptimizer
    imgui
)