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


add_executable (xihe WIN32 "xihe_app.cpp" "xihe_app.h" "backend/instance.h" "backend/instance.cpp" "platform/window.h" "platform/window.cpp" "common/logging.h" "common/error.h" "common/error.cpp" "common/strings.h" "common/strings.cpp" "platform/glfw_window.h" "platform/glfw_window.cpp" "backend/debug.h" "backend/debug.cpp" "backend/physical_device.h" "backend/physical_device.cpp" "backend/device.h" "backend/device.cpp" "backend/vulkan_resource.h" "backend/vulkan_resource.cpp" "backend/resources_management/resource_cache.h" "backend/resources_management/resource_cache.cpp" "backend/queue.h" "backend/queue.cpp" "backend/command_pool.h" "backend/command_pool.cpp" "backend/command_buffer.h" "backend/command_buffer.cpp" "backend/fence_pool.h" "backend/fence_pool.cpp" "rendering/render_context.h" "rendering/render_context.cpp" "backend/swapchain.h" "backend/swapchain.cpp" "rendering/render_target.h" "rendering/render_target.cpp" "backend/image.h" "backend/image.cpp" "rendering/render_frame.h" "rendering/render_frame.cpp" "backend/descriptor_pool.h" "backend/descriptor_pool.cpp" "backend/descriptor_set_layout.h" "backend/descriptor_set_layout.cpp" "backend/buffer_pool.h" "backend/buffer_pool.cpp" "backend/descriptor_set.h" "backend/descriptor_set.cpp" "backend/semaphore_pool.h" "backend/semaphore_pool.cpp" "main.cpp" "platform/platform.h" "platform/platform.cpp" "platform/windows/windows_platform.h" "platform/windows/windows_platform.cpp" "platform/input_events.h" "platform/application.h" "platform/application.cpp" "common/timer.h" "common/timer.cpp" "common/vk_common.h" "common/vk_common.cpp" "backend/image_view.h" "backend/image_view.cpp" "platform/input_events.cpp" "backend/shader_module.h" "backend/shader_module.cpp" "platform/filesystem.h" "platform/filesystem.cpp" "backend/shader_compiler/glsl_compiler.h" "backend/shader_compiler/glsl_compiler.cpp" "backend/shader_compiler/spirv_reflection.h" "backend/shader_compiler/spirv_reflection.cpp" "common/helpers.h" "backend/pipeline_layout.h" "backend/pipeline_layout.cpp" "backend/render_pass.h" "backend/render_pass.cpp" "backend/pipeline.h" "backend/pipeline.cpp" "rendering/pipeline_state.h" "rendering/pipeline_state.cpp" "backend/framebuffer.h" "backend/framebuffer.cpp" "backend/resources_management/resource_record.h" "backend/resources_management/resource_record.cpp" "backend/resources_management/resource_caching.h" "common/glm_common.h")

#if (CMAKE_VERSION VERSION_GREATER 3.12)
set_property(TARGET xihe PROPERTY CXX_STANDARD 20)
#endif()

# target_compile_definitions(xihe PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Link third party libraries
target_link_libraries(xihe PUBLIC
    volk
    glm
    spdlog
    glfw
    vma
    glslang
    SPIRV
    spirv-cross-glsl
    glslang-default-resource-limits
)