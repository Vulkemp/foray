cmake_minimum_required(VERSION 3.20)

set(app "importancesamplingrt")

file(GLOB_RECURSE src "*.cpp")

add_executable(${app} ${src})

target_link_libraries(
	${app}
	PUBLIC ${PROJECT_NAME}
)


target_include_directories(
	${app}
	PUBLIC "../../src"
	PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${thirdparty_include_dir}
)