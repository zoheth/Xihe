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

# ctpl
add_library(ctpl INTERFACE)
set(CTPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CTPL)
target_sources(ctpl INTERFACE ${CTPL_DIR}/ctpl_stl.h)
target_include_directories(ctpl SYSTEM INTERFACE ${CTPL_DIR})

# enkiTS
add_library(enkits INTERFACE)
set(ENKITS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/enkiTS/src)
target_sources(enkits INTERFACE ${ENKITS_DIR}/TaskScheduler.cpp ${ENKITS_DIR}/TaskScheduler.h ${ENKITS_DIR}/LockLessMultiReadPipe.h)
target_include_directories(enkits SYSTEM INTERFACE ${ENKITS_DIR})

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


# imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h")

add_library(imgui STATIC ${IMGUI_FILES})

target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR})

set_target_properties(imgui PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# libktx
option(KTX_FEATURE_TESTS "" OFF)
option(KTX_FEATURE_TOOLS "" OFF)
option(KTX_FEATURE_STATIC_LIBRARY "" ON) 
add_subdirectory(ktx)

# set_target_properties(ktx PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
set_target_properties(ktx PROPERTIES FOLDER "ThirdParty")
set_target_properties(ktx_version PROPERTIES FOLDER "ThirdParty")

# stb
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")

# Check if the ASTC_ARCH is not defined by the user
if (NOT DEFINED ASTC_ARCH)
    if (APPLE)
        # Assume native architecture on Apple systems, especially for ARM based Macs
        set(ASTC_ARCH NATIVE)
    else()
        # Use CMake's built-in features to check processor capabilities
        include(CheckIncludeFileCXX)
        include(CheckCXXSourceCompiles)
        
        # Check for AVX2 support
        check_cxx_source_compiles("
            #include <immintrin.h>
            int main() {
                __m256i a = _mm256_set1_epi32(0);
                __m256i b = _mm256_set1_epi32(1);
                __m256i c = _mm256_add_epi32(a, b);
                return 0;
            }
        " ASTC_ARCH_AVX2)

        # Set the architecture based on availability
        if (ASTC_ARCH_AVX2)
            set(ASTC_ARCH AVX2)
        else()
            set(ASTC_ARCH NATIVE)
        endif()
    endif()
endif()

string(TOUPPER ${ASTC_ARCH} ASTC_ARCH)
string(TOLOWER ${ASTC_ARCH} ASTC_ARCH_LOWER)

set(ASTCENC_ISA_${ASTC_ARCH} ON)
set(ASTCENC_CLI OFF)
set(ASTCENC_UNITTEST OFF)
set(ASTCENC_DECOMPRESSOR ON)
set(ASTCENC_UNIVERSAL_BUILD OFF)
set(ASTC_RAW_TARGET astcdec-${ASTC_ARCH_LOWER}-static)
set(ASTC_TARGET ${ASTC_RAW_TARGET} PARENT_SCOPE)

# astc
add_subdirectory(astc)

# ASTC apparently tries to build for x86_64 even on Mac arm64 architectures,
# but we can force it to build for the correct arch
# Upstream bug: https://github.com/ARM-software/astc-encoder/issues/458
if(IOS AND (CMAKE_SYSTEM_PROCESSOR STREQUAL "") )
    set(CMAKE_SYSTEM_PROCESSOR "arm64")
endif ()
if (APPLE AND (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64"))
    set_target_properties(${ASTC_RAW_TARGET} PROPERTIES OSX_ARCHITECTURES "arm64")
endif()

# if targeting iOS, that's always arm64 even when the host OS is x86-64
if(IOS)
    set_target_properties(${ASTC_RAW_TARGET} PROPERTIES OSX_ARCHITECTURES "arm64")
endif ()

# astc doesn't have separate directories for it's source code and public interface.  Additionally, it includes it's
# own copy of STB. In order to avoid conflicts, we copy the only header we need to the build directory and alter the
# INTERFACE_INCLUDE_DIRECTORIES of the target
if (NOT TARGET ${ASTC_RAW_TARGET})
    message(FATAL_ERROR "Couldn't find expected ASTC library target")
endif()

get_target_property(ASTC_BINARY_DIR ${ASTC_RAW_TARGET} BINARY_DIR)
file(
    COPY astc/Source/astcenc.h
    DESTINATION ${ASTC_BINARY_DIR}/Include)

    set_target_properties(${ASTC_RAW_TARGET} PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ""
    INCLUDE_DIRECTORIES ""
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${ASTC_BINARY_DIR}/Include
)
target_include_directories(${ASTC_RAW_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/astc/Source)
target_include_directories(${ASTC_RAW_TARGET} SYSTEM INTERFACE  ${ASTC_BINARY_DIR}/Include)

if (MSVC)
  target_compile_definitions(${ASTC_RAW_TARGET} PRIVATE _CRT_SECURE_NO_WARNINGS)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(gtest PUBLIC -Wno-deprecated-declarations)
  endif ()
endif ()
set_property(TARGET ${ASTC_RAW_TARGET} PROPERTY FOLDER "ThirdParty")

# meshoptimizer
add_subdirectory(meshoptimizer)
set_property(TARGET meshoptimizer PROPERTY FOLDER "ThirdParty")
