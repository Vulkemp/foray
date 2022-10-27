cmake_minimum_required(VERSION 3.20)

project("foray")

# Include Compiler Config (sets c++ 20 and compiler flags)
include("cmakescripts/compilerconfig.cmake")
include("cmakescripts/compileshader.cmake")

# Enable strict mode for own lib
SET(CMAKE_CXX_FLAGS ${STRICT_FLAGS})

# Gather sources
file(GLOB_RECURSE src "src/*.cpp")

option(DISABLE_RT_EXTENSIONS "If set, will disable raytracing extensions" OFF)
if (DISABLE_RT_EXTENSIONS)
    add_compile_definitions(DISABLE_RT_EXTENSIONS)
endif()
option(ENABLE_GBUFFER_BENCH "If enabled, will perform a device level benchmark in the gbuffer stage" OFF)
if (ENABLE_GBUFFER_BENCH)
    add_compile_definitions(ENABLE_GBUFFER_BENCH)
endif()
add_library(${PROJECT_NAME} STATIC ${src} )

# dependencies

find_package(Vulkan REQUIRED)

set(SDL2_HINT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
include("cmakescripts/locatesdl2.cmake") # Find SDL either by find_package or as a fallback the included version

# manual dependency variables

set(thirdparty_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(thirdparty_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# link dependency libs
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC ${Vulkan_LIBRARIES}
    PUBLIC glm_static
    PUBLIC vkbootstrap
    PUBLIC vma
    PUBLIC tinygltf
    PUBLIC tinyexr
    PUBLIC imgui
    PUBLIC ${SDL2_LIBRARIES}
    )

# include directories

include_directories(
	PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${SDL2_INCLUDE_DIRS}
    PUBLIC ${thirdparty_include_dir}
)

# target_precompile_headers(${PROJECT_NAME} PRIVATE "src/foray_precompile.hpp")

# Compile Definitions
if (DISABLE_RT_EXTENSIONS) 
    target_compile_definitions(${PROJECT_NAME} PUBLIC DISABLE_RT_EXTENSIONS)
endif ()


# subdirectories

add_subdirectory("src/shaders")

# Set nonstrict mode for third party stuff

SET(CMAKE_CXX_FLAGS ${NONSTRICT_FLAGS})

add_subdirectory("${thirdparty_dir}/vma")
add_subdirectory("${thirdparty_dir}/vkbootstrap")
add_subdirectory("${thirdparty_dir}/glm")
add_subdirectory("${thirdparty_dir}/tinygltf")
add_subdirectory("${thirdparty_dir}/tinyexr")
add_subdirectory("${thirdparty_dir}/imgui")