# Use C++ 20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BASE_FLAGS "${CMAKE_CXX_FLAGS}")

# Compiler flags
if (CMAKE_COMPILER_IS_GNUCXX MATCHES TRUE)

    # See https://gcc.gnu.org/onlinedocs/gcc/Option-Index.html

    set(NONSTRICT_FLAGS "${BASE_FLAGS} -w")
    set(STRICT_FLAGS "${BASE_FLAGS} -Wall")

elseif (MSVC MATCHES TRUE)

    # See https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-by-category?view=msvc-170

    set(NONSTRICT_FLAGS "${BASE_FLAGS} /w") # Disables all warnings
    set(STRICT_FLAGS "${BASE_FLAGS} /Wall") # Maximum warning level

endif()
