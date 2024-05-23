cmake_minimum_required(VERSION 3.10)

project(third_party LANGUAGES C CXX)

# glm
add_library(glm INTERFACE)
set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
target_sources(glm INTERFACE ${GLM_DIR}/glm/glm.hpp)
target_include_directories(glm SYSTEM INTERFACE ${GLM_DIR})

target_compile_definitions(glm INTERFACE
    GLM_FORCE_SWIZZLE
    GLM_FORCE_RADIANS
    GLM_FORCE_CTOR_INIT
    GLM_ENABLE_EXPERIMENTAL
)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(glm INTERFACE GLM_FORCE_CXX14)
endif()

# vulkan
add_library(vulkan INTERFACE)
set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
target_include_directories(vulkan INTERFACE ${VULKAN_INCLUDE_DIR})
target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)
target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WIN32_KHR)

# volk
set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
set(VOLK_FILES
    "${VOLK_DIR}/volk.c"
    "${VOLK_DIR}/volk.h")

add_library(volk STATIC ${VOLK_FILES})
set_target_properties(volk PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(volk PUBLIC vulkan)

target_include_directories(volk PUBLIC ${VOLK_DIR})
if (VKB_WSI_SELECTION STREQUAL XCB)
    target_include_directories(volk PUBLIC ${XCB_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL XLIB)
    target_include_directories(volk PUBLIC ${X11_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
    target_include_directories(volk PUBLIC ${WAYLAND_INCLUDE_DIRS})
endif()

set_property(TARGET volk PROPERTY FOLDER "ThirdParty")

# vma
add_library(vma INTERFACE)
set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vma/include)
target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
target_include_directories(vma SYSTEM INTERFACE ${VMA_DIR})
target_link_libraries(vma INTERFACE vulkan)

# tinygltf
add_library(tinygltf INTERFACE)
set(TINYGLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
target_sources(tinygltf INTERFACE ${TINYGLTF_DIR}/tiny_gltf.h ${TINYGLTF_DIR}/json.hpp)
target_include_directories(tinygltf SYSTEM INTERFACE ${TINYGLTF_DIR})

# fmt
add_subdirectory(fmt)
set_property(TARGET fmt PROPERTY FOLDER "ThirdParty")

# spdlog
set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory(spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "ThirdParty")

# GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)
option(BUILD_SHARED_LIBS ON)
if (VKB_WSI_SELECTION STREQUAL WAYLAND)
    set(GLFW_USE_WAYLAND ON)
endif()

add_subdirectory(glfw)

set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")

# glslang
option(ENABLE_SPVREMAPPER OFF)
option(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_HLSL ON)
option(ENABLE_OPT OFF)
option(BUILD_TESTING OFF)
option(BUILD_EXTERNAL OFF)

add_subdirectory(glslang)

if (NOT TARGET glslang-default-resource-limits)
    add_library(glslang-default-resource-limits
                glslang/StandAlone/ResourceLimits.cpp)

    set_target_properties(glslang-default-resource-limits PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

    target_include_directories(glslang-default-resource-limits
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
endif()

if(NOT MSVC)
    target_compile_options(glslang PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")

    target_compile_options(SPIRV PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")
endif()

set_target_properties(glslang PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(OGLCompiler PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(OSDependent PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(SPIRV PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# spirv-cross
add_subdirectory(spirv-cross)

set_target_properties(spirv-cross PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-core PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(spirv-cross-util PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
