project(xihe LANGUAGES C CXX)

option(USE_VULKAN_DEBUG "Use Vulkan debug utilities" ON)

if(USE_VULKAN_DEBUG)
  add_compile_definitions(XH_VULKAN_DEBUG)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable (xihe "xihe_app.cpp" "xihe_app.h" "backend/instance.h" "backend/instance.cpp" "platform/window.h" "platform/window.cpp" "common/logging.h" "common/error.h" "common/error.cpp" "common/strings.h" "common/strings.cpp" "platform/glfw_window.h" "platform/glfw_window.cpp" "backend/debug.h" "backend/debug.cpp" "backend/physical_device.h" "backend/physical_device.cpp" "backend/device.h" "backend/device.cpp" "backend/vulkan_resource.h" "backend/vulkan_resource.cpp" "backend/resource_cache.h" "backend/resource_cache.cpp" "backend/queue.h" "backend/queue.cpp" "backend/command_pool.h" "backend/command_pool.cpp" "backend/command_buffer.h" "backend/command_buffer.cpp" "backend/fence_pool.h" "backend/fence_pool.cpp" "rendering/render_context.h" "rendering/render_context.cpp" "backend/swapchain.h" "backend/swapchain.cpp" "rendering/render_target.h" "rendering/render_target.cpp" "backend/image.h" "backend/image.cpp" "rendering/render_frame.h" "rendering/render_frame.cpp")

#if (CMAKE_VERSION VERSION_GREATER 3.12)
set_property(TARGET xihe PROPERTY CXX_STANDARD 20)
#endif()

# Link third party libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    volk
    spdlog
    glfw
    vma
)