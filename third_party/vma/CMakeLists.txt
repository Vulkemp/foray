cmake_minimum_required(VERSION 3.18)

include_directories(
	PUBLIC ${Vulkan_INCLUDE_DIRS}
)

add_library(vma STATIC "vma.cpp")

# Disable compile warnings for vma
set_target_properties(vma PROPERTIES COMPILE_FLAGS ${NONSTRICT_FLAGS})
