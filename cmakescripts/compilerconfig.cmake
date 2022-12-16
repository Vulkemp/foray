# Use C++ 20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # using Clang
    # See https://clang.llvm.org/docs/DiagnosticsReference.html

    set(NONSTRICT_FLAGS "-w -Wno-nullability-completeness")
    set(STRICT_FLAGS "-Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    # See https://gcc.gnu.org/onlinedocs/gcc/Option-Index.html

    set(NONSTRICT_FLAGS "-w")
    set(STRICT_FLAGS "-Wall")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
    # See https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-by-category?view=msvc-170
    
    set(NONSTRICT_FLAGS "/w") # Disables all warnings
    set(STRICT_FLAGS "/W2") # Maximum warning level

endif()

# How to detect compiler
# https://stackoverflow.com/questions/10046114/in-cmake-how-can-i-test-if-the-compiler-is-clang
