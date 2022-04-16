cmake_minimum_required(VERSION 3.18)

include_directories(
	PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${thirdparty_include_dir}
)



add_library(vkglTF STATIC "VulkanglTFModel.cpp")