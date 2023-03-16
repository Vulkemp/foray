cmake_minimum_required(VERSION 3.18)

include_directories(
	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../sdl2/"
	 ${Vulkan_INCLUDE_DIRS}
)

add_library(imgui STATIC)

target_sources(imgui PUBLIC

"${CMAKE_CURRENT_SOURCE_DIR}/imgui.h"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_vulkan.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_sdl.cpp"

)