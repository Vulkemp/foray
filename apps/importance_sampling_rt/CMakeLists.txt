cmake_minimum_required(VERSION 3.20)

set(app "importancesamplingrt")

file(GLOB_RECURSE src "*.cpp")

add_executable(${app} ${src})

target_link_libraries(
	${app}
	PUBLIC ${PROJECT_NAME}
)

# TODO: Make this file prettier
function(hsk_postbuild_copy target_name sourcepath targetdir)
# https://stackoverflow.com/questions/10671916/how-to-copy-dll-files-into-the-same-folder-as-the-executable-using-cmake
    
    message(INFO "target_name add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${sourcepath}
        ${targetdir})")
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${sourcepath}
        ${targetdir})
endfunction()

if (WIN32)
    # copy sdl2.dll to executable dir
    hsk_postbuild_copy(${app} ${sdl2_dll} "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(${app} PUBLIC ${sdl2_libmain})
endif()

target_include_directories(
	${app}
	PUBLIC "../../src"
	PUBLIC ${Vulkan_INCLUDE_DIRS}
    PUBLIC ${thirdparty_include_dir}
)

message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(${app}
  PUBLIC "${CMAKE_SOURCE_DIR}/src/hsk_rtrpf.hpp")