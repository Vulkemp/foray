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

# manual dependency variables

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party" CACHE PATH "Third party libraries directory")

include("cmakescripts/locatesdl2.cmake") # Find SDL either by find_package or as a fallback the included version

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

add_subdirectory("$CACHE{THIRD_PARTY_DIR}/vma")
add_subdirectory("$CACHE{THIRD_PARTY_DIR}/vkbootstrap")
add_subdirectory("$CACHE{THIRD_PARTY_DIR}/glm")
add_subdirectory("$CACHE{THIRD_PARTY_DIR}/tinygltf")
add_subdirectory("$CACHE{THIRD_PARTY_DIR}/tinyexr")
add_subdirectory("$CACHE{THIRD_PARTY_DIR}/imgui")

# Set cache entry to shader dir

set(FORAY_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders" CACHE PATH "Foray shader directory")

# Precompiled header for third party libraries

option(FORAY_PRECOMPILE_THIRDPARTY_HEADERS "Enables packing third party includes into a precompiled header. Helps a little with build times." ON)
if (FORAY_PRECOMPILE_THIRDPARTY_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/foray_precompile.hpp")
endif()

option(FORAY_CATCH_EXCEPTIONS "Sets exception catch mode: OFF = No central exceptions catcher. Aids live debugging, but won't print error message to console. ON = Catch exceptions centrally and log error message." OFF)
if (FORAY_CATCH_EXCEPTIONS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "FORAY_CATCH_EXCEPTIONS=1")
endif()

option(FORAY_DISABLE_RT "Disables calls to RT features in areas which do not strictly need it, such as AS building in scene. For when you want to work on a device which does not support RT." OFF)
if (FORAY_DISABLE_RT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "FORAY_DISABLE_RT=1")
endif()
