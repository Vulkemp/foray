# This function is used to provide shaders packed directly into the binary by invoking the GLSLC executable bundled with Vulkan SDK
function(foray_compileshader SrcPath DstPath)

# Make sure GLSLC Exe is available
if (NOT Vulkan_GLSLC_EXECUTABLE)
    message(FATAL_ERROR "Vulkan Package must be located before using the compileshader(...) can be used")
endif()

# Make sure source file exists
if (NOT EXISTS ${SrcPath})
    message(FATAL_ERROR "Shader compile source does not exist \"${SrcPath}\"")
endif()

# Check if output file is older than source file, if yes compilation is skipped
if (EXISTS ${DstPath})
    file(TIMESTAMP ${SrcPath} SRC_TIMESTAMP "%s" UTC)
    file(TIMESTAMP ${DstPath} DST_TIMESTAMP "%s" UTC)
    if (DST_TIMESTAMP GREATER SRC_TIMESTAMP)
        message(VERBOSE "Skipping shader compile of \"${SrcPath}\"")
        return()
    endif()
endif()

# call GLSLC
execute_process(COMMAND ${Vulkan_GLSLC_EXECUTABLE} -O -mfmt=c -o ${DstPath} ${SrcPath} # -O Optimize; -mfmt=c Output as C style uint32_t array -o Specify output file
    RESULT_VARIABLE RESULT # Store command execution result (integer, 0 = compilation succeeded)
    OUTPUT_VARIABLE GLSLC_OUTPUT # Catch output
    ERROR_VARIABLE GLSLC_OUTPUT) # Catch errors

# Confirm result is success
if (NOT RESULT EQUAL 0)
    set(GLSLC_COMMAND "${Vulkan_GLSLC_EXECUTABLE} -O -mfmt=c -o \"${DstPath}\" \"${SrcPath}\"")
    message(FATAL_ERROR "Shader Compilation Failed (${RESULT})! Command: ${GLSLC_COMMAND}\nGLSLC stdout: ${GLSLC_OUTPUT}")
endif ()

endfunction()
