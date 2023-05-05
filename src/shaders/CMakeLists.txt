include("../../cmakescripts/compileshader.cmake")

set(STAGE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../stages")

# GBuffer shaders are packed as spv binary code into library
foray_CompileShader("${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/gbuffer_stage.vert" "${STAGE_SRC_DIR}/foray_gbuffer.vert.spv.h")
foray_CompileShader("${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/gbuffer_stage.frag" "${STAGE_SRC_DIR}/foray_gbuffer.frag.spv.h")

# Comparer stage compute shaders are packed as spv binary code into library
foray_CompileShader("${CMAKE_CURRENT_SOURCE_DIR}/comparerstage/comparerstage.f.comp" "${STAGE_SRC_DIR}/foray_comparerstage.f.comp.spv.h")
foray_CompileShader("${CMAKE_CURRENT_SOURCE_DIR}/comparerstage/comparerstage.i.comp" "${STAGE_SRC_DIR}/foray_comparerstage.i.comp.spv.h")
foray_CompileShader("${CMAKE_CURRENT_SOURCE_DIR}/comparerstage/comparerstage.u.comp" "${STAGE_SRC_DIR}/foray_comparerstage.u.comp.spv.h")
