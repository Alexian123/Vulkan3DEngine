cmake_minimum_required(VERSION 3.21)
project(Vulkan3DEngine LANGUAGES CXX)

# ============================================================
# Build settings
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable folders in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ============================================================
# Source files
# ============================================================
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
add_executable(Vulkan3DEngine ${ENGINE_SOURCES})

# ============================================================
# External libraries (relative paths)
# ============================================================
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(GLFW_ROOT      ${EXTERNAL_DIR}/glfw-3.4.bin.WIN64)
set(GLM_ROOT       ${EXTERNAL_DIR}/glm-1.0.2)
set(STB_ROOT       ${EXTERNAL_DIR}/stb_image)
set(TINYOBJ_ROOT   ${EXTERNAL_DIR}/tinyobjloader)

target_include_directories(Vulkan3DEngine PRIVATE
    src
    include
    ${GLFW_ROOT}/include
    ${GLM_ROOT}
    ${STB_ROOT}
    ${TINYOBJ_ROOT}
)

if(WIN32)
    target_link_directories(Vulkan3DEngine PRIVATE ${GLFW_ROOT}/lib-vc2022)
    target_link_libraries(Vulkan3DEngine PRIVATE glfw3)
endif()

# ============================================================
# Vulkan
# ============================================================
find_package(Vulkan REQUIRED)
target_link_libraries(Vulkan3DEngine PRIVATE Vulkan::Vulkan)

# ============================================================
# Windows macros and compiler options
# ============================================================
if(MSVC)
    target_compile_definitions(Vulkan3DEngine PRIVATE NOMINMAX)
    target_compile_options(Vulkan3DEngine PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(Vulkan3DEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ============================================================
# Copy resources folder to output
# ============================================================
add_custom_command(TARGET Vulkan3DEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:Vulkan3DEngine>/resources
)

# ============================================================
# Shader compilation
# ============================================================
set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${SHADER_SRC_DIR}/compiled)

# Allow users to set VulkanSDK manually or use environment
if(NOT DEFINED VulkanSDK)
    set(VulkanSDK $ENV{VULKAN_SDK})
endif()

if(NOT VulkanSDK)
    message(FATAL_ERROR "VULKAN_SDK not set! Please install Vulkan SDK and set VULKAN_SDK environment variable.")
endif()

set(GLSLC_EXE "${VulkanSDK}/Bin/glslc.exe")

# List of shaders (vertex and fragment)
set(SHADER_FILES
    simple.vert
    simple.frag
    point_light.vert
    point_light.frag
)

set(SPV_FILES "")
foreach(SHADER ${SHADER_FILES})
    get_filename_component(NAME_WE ${SHADER} NAME_WE)
    get_filename_component(EXT ${SHADER} EXT)

    if(EXT STREQUAL ".vert")
        set(OUT_FILE ${SHADER_OUT_DIR}/${NAME_WE}_vert.spv)
    elseif(EXT STREQUAL ".frag")
        set(OUT_FILE ${SHADER_OUT_DIR}/${NAME_WE}_frag.spv)
    else()
        message(FATAL_ERROR "Unknown shader extension: ${SHADER}")
    endif()

    add_custom_command(
        OUTPUT ${OUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
        COMMAND ${GLSLC_EXE} ${SHADER_SRC_DIR}/${SHADER} -o ${OUT_FILE}
        DEPENDS ${SHADER_SRC_DIR}/${SHADER}
        COMMENT "Compiling shader ${SHADER}"
        VERBATIM
    )

    list(APPEND SPV_FILES ${OUT_FILE})
endforeach()

# Make sure shaders are compiled before the executable
add_custom_target(CompileShaders ALL DEPENDS ${SPV_FILES})
add_dependencies(Vulkan3DEngine CompileShaders)

# ============================================================
# Output directories
# ============================================================
set_target_properties(Vulkan3DEngine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# ============================================================
# Platform-specific definitions
# ============================================================
if(WIN32)
    target_compile_definitions(Vulkan3DEngine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

# Copy compiled shaders to the executable output folder
add_custom_command(TARGET Vulkan3DEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_OUT_DIR}
        $<TARGET_FILE_DIR:Vulkan3DEngine>/shaders
)

message(STATUS "Vulkan3DEngine configured successfully!")
message(STATUS "Resources and compiled shaders will be copied automatically.")
