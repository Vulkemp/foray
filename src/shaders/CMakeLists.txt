include("../../cmakescripts/compileshader.cmake")

set(STAGE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../stages")

# GBuffer shaders are packed as spv binary code into library
foray_compileshader("${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/gbuffer_stage.vert" "${STAGE_SRC_DIR}/foray_gbuffer.vert.spv.h")
foray_compileshader("${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/gbuffer_stage.frag" "${STAGE_SRC_DIR}/foray_gbuffer.frag.spv.h")