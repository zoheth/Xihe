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

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_executable (xihe WIN32 ${HEADER_FILES} ${SOURCE_FILES})

set_property(TARGET xihe PROPERTY CXX_STANDARD 20)

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