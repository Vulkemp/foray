cmake_minimum_required(VERSION 3.20)

project("foray")

# Include Compiler Config (sets c++ 20 and compiler flags)
include("cmakescripts/compilerconfig.cmake")
include("cmakescripts/compileshader.cmake")

# Gather sources
file(GLOB_RECURSE src "src/*.cpp")

add_library(${PROJECT_NAME} STATIC ${src} )

# Set strict mode for project only
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${STRICT_FLAGS})

# dependencies

find_package(Vulkan REQUIRED)

include("cmakescripts/locatesdl2.cmake") # Find SDL either by find_package or as a fallback the included version

# manual dependency variables

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

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
    )
include_directories(SYSTEM
    PUBLIC ${THIRD_PARTY_DIR}
)

# subdirectories

add_subdirectory("src/shaders")

# Set nonstrict mode for third party stuff
set_property(GLOBAL PROPERTY COMPILE_FLAGS ${NONSTRICT_FLAGS})

add_subdirectory("${THIRD_PARTY_DIR}/vma")
add_subdirectory("${THIRD_PARTY_DIR}/vkbootstrap")
add_subdirectory("${THIRD_PARTY_DIR}/glm")
add_subdirectory("${THIRD_PARTY_DIR}/tinygltf")
add_subdirectory("${THIRD_PARTY_DIR}/tinyexr")
add_subdirectory("${THIRD_PARTY_DIR}/imgui")

# Set cache entry to shader dir

set(FORAY_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders" CACHE PATH "Foray shader directory")

# Precompiled header for third party libraries

option(FORAY_PRECOMPILE_THIRDPARTY_HEADERS "Enables packing third party includes into a precompiled header. Helps a little with build times." ON)
if (FORAY_PRECOMPILE_THIRDPARTY_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/foray_precompile.hpp")
endif()
