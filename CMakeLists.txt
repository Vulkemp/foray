cmake_minimum_required(VERSION 3.20)

project("rtrpf")

# declare lib

file(GLOB_RECURSE src "src/*.cpp")
add_library(${PROJECT_NAME} ${src} )

# dependencies

find_package(Vulkan REQUIRED)

# manual dependency variables

set(thirdparty_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(thirdparty_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(apps_dir "${CMAKE_CURRENT_SOURCE_DIR}/apps")
set(sdl2_dll "${thirdparty_dir}/sdl2/win/x64/SDL2.dll")

# link dependency libs

if (WIN32) 
    target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC ${Vulkan_LIBRARIES}
        PUBLIC glm_static
        PUBLIC vkbootstrap
        PUBLIC "${thirdparty_dir}/sdl2/win/x64/SDL2.lib")
else () 
    target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC -lvulkan
        PUBLIC glm_static
        PUBLIC vkbootstrap
        PUBLIC -lSDL2)
endif()

# include directories

include_directories(
	PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${thirdparty_include_dir}
)

target_compile_definitions(${PROJECT_NAME} PUBLIC HSK_RTRPF_EXPORT)

# subdirectories

add_subdirectory("${thirdparty_dir}/vkbootstrap")
add_subdirectory("${thirdparty_dir}/glm")

# applications

add_subdirectory("${apps_dir}/importance_sampling_rt")