cmake_minimum_required(VERSION 3.20)

project("importance_sampling_rt")

file(GLOB_RECURSE src "*.cpp")

add_executable(${PROJECT_NAME} ${src})

target_include_directories(
	${PROJECT_NAME}
	PUBLIC "../../src"
	PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${thirdparty_include_dir}
)