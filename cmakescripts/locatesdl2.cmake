# Attempt to locate via sdl2-config.cmake which should have been installed together with SDL2
find_package(SDL2)

if (UNIX AND NOT DEFINED SDL2_INCLUDE_DIRS)
    # On unix systems, SDL should be installed via package manager
    MESSAGE(FATAL_ERROR "SDL2 not found! Install SDL dev package via your preferred package manager!")
endif()

# If that fails: Use builtin SDL2 binaries
if (NOT DEFINED SDL2_INCLUDE_DIRS OR NOT DEFINED SDL2_LIBRARIES)
    SET(SDL2_BASE_DIR "../third_party/sdl2")
    SET(SDL2_INCLUDE_DIRS "../third_party")

    # https://stackoverflow.com/questions/39258250/how-to-detect-if-64-bit-msvc-with-cmake
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64 bit config
        SET(SDL2_LIBRARIES_DIR "${SDL2_BASE_DIR}/win/x64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        # 32 bit config
        SET(SDL2_LIBRARIES_DIR "${SDL2_BASE_DIR}/win/x86")
    endif()
    SET(SDL2_LIBRARIES "${SDL2_LIBRARIES_DIR}/SDL2.lib" "${SDL2_LIBRARIES_DIR}/SDL2main.lib")
    SET(SDL2_DLL "${SDL2_LIBRARIES_DIR}/SDL2.dll")
endif()