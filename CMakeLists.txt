cmake_minimum_required(VERSION 3.7)

project("rtrpf")

# Include Compiler Config (sets c++ 20 and compiler flags)
include("cmakescripts/compilerconfig.cmake")

# Enable strict mode for own lib
SET(CMAKE_CXX_FLAGS ${STRICT_FLAGS})

# Gather sources
file(GLOB_RECURSE src "src/*.cpp")

option(DISABLE_RT_EXTENSIONS 0)
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

target_precompile_headers(${PROJECT_NAME}
    PRIVATE "src/hsk_precompile.hpp")

# Compile Definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC HSK_RTRPF_EXPORT)
if (DISABLE_RT_EXTENSIONS) 
    target_compile_definitions(${PROJECT_NAME} PUBLIC DISABLE_RT_EXTENSIONS)
endif ()


# subdirectories

# Set nonstrict mode for third party stuff

SET(CMAKE_CXX_FLAGS ${NONSTRICT_FLAGS})

add_subdirectory("${thirdparty_dir}/vma")
add_subdirectory("${thirdparty_dir}/vkbootstrap")
add_subdirectory("${thirdparty_dir}/glm")
add_subdirectory("${thirdparty_dir}/tinygltf")
add_subdirectory("${thirdparty_dir}/tinyexr")
add_subdirectory("${thirdparty_dir}/imgui")