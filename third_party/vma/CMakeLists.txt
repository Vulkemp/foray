cmake_minimum_required(VERSION 3.18)

include_directories(
	PUBLIC ${Vulkan_INCLUDE_DIRS}
)

add_library(vma STATIC "vma.cpp")