cmake_minimum_required(VERSION 3.20)

add_subdirectory("vma")
# add_subdirectory("vkbootstrap") We don't use this because bruh
add_subdirectory("glm")
add_subdirectory("tinygltf")
add_subdirectory("tinyexr")
add_subdirectory("spdlog")

# build include dir list

set(LIST $CACHE{FORAY_INCLUDE_DIR_LIST})

list(APPEND LIST 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/nameof/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/glm"
)



set(FORAY_INCLUDE_DIR_LIST LIST CACHE)

##### IMGUI #####

add_library(imgui STATIC)

target_sources(imgui
    PUBLIC "imgui/imgui.cpp"
    PUBLIC "imgui/imgui_demo.cpp"
    PUBLIC "imgui/imgui_draw.cpp"
    PUBLIC "imgui/imgui_widgets.cpp"
    PUBLIC "imgui/imgui_tables.cpp"
    PUBLIC "imgui/backends/imgui_impl_vulkan.cpp"
    PUBLIC "imgui/backends/imgui_impl_sdl2.cpp"
)

if (UNIX)
    target_include_directories(imgui
        PUBLIC "./imgui/"
        PUBLIC ${Vulkan_INCLUDE_DIRS}
        )
        else()
        target_include_directories(imgui
    	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/"
        PUBLIC "./imgui/"
    	PUBLIC ${Vulkan_INCLUDE_DIRS}
    )
endif()
