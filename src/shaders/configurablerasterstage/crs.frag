#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define INTERFACEMODE in
#include "shaderinterface.glsl"

#if OUT_0
layout(location = 0) out OUT_0_TYPE out0;
#endif
#if OUT_1
layout(location = 1) out OUT_1_TYPE out1;
#endif
#if OUT_2
layout(location = 2) out OUT_2_TYPE out2;
#endif
#if OUT_3
layout(location = 3) out OUT_3_TYPE out3;
#endif
#if OUT_4
layout(location = 4) out OUT_4_TYPE out4;
#endif
#if OUT_5
layout(location = 5) out OUT_5_TYPE out5;
#endif
#if OUT_6
layout(location = 6) out OUT_6_TYPE out6;
#endif
#if OUT_7
layout(location = 7) out OUT_7_TYPE out7;
#endif
#if OUT_8
layout(location = 8) out OUT_8_TYPE out8;
#endif
#if OUT_9
layout(location = 9) out OUT_9_TYPE out9;
#endif
#if OUT_10
layout(location = 10) out OUT_10_TYPE out10;
#endif
#if OUT_11
layout(location = 11) out OUT_11_TYPE out11;
#endif
#if OUT_12
layout(location = 12) out OUT_12_TYPE out12;
#endif
#if OUT_13
layout(location = 13) out OUT_13_TYPE out13;
#endif
#if OUT_14
layout(location = 14) out OUT_14_TYPE out14;
#endif
#if OUT_15
layout(location = 15) out OUT_15_TYPE out15;
#endif

#include "bindpoints.glsl"
#include "../common/gltf_pushc.glsl"
#include "../common/materialbuffer.glsl"
#include "../common/normaltbn.glsl"

void main()
{
#if MATERIALPROBE || NORMALMAPPING
    MaterialBufferObject material = GetMaterialOrFallback(PushConstant.MaterialIndex);
    #define EXISTS_MATERIAL 1

    MaterialProbe probe = ProbeMaterial(material, UV);
    #define EXISTS_PROBE 1
#endif
#if MATERIALPROBEALPHA || ALPHATEST
    #if EXISTS_PROBE
        bool isOpaque = probe.BaseColor.a > 0.f;
        #define EXISTS_ISOPAQUE 1
    #else
    #if !EXISTS_MATERIAL
        MaterialBufferObject material = GetMaterialOrFallback(PushConstant.MaterialIndex);
        #define EXISTS_MATERIAL 1
    #endif

        bool isOpaque = ProbeAlphaOpacity(material, UV);
        #define EXISTS_ISOPAQUE 1
    #endif
#endif
#if ALPHATEST
    if (!isOpaque)
    {
        discard;
    }
#endif
#if NORMALMAPPING
    vec3 normalMapped = ApplyNormalMap(CalculateTBN(Normal, Tangent), probe);
    #define EXISTS_NORMALMAPPED 1
#endif

#if OUT_0
    OUT_0_CALC
    out0 = OUT_0_TYPE(OUT_0_RESULT);
#endif
#if OUT_1
    OUT_1_CALC
    out1 = OUT_1_TYPE(OUT_1_RESULT);
#endif
#if OUT_2
    OUT_2_CALC
    out2 = OUT_2_TYPE(OUT_2_RESULT);
#endif
#if OUT_3
    OUT_3_CALC
    out3 = OUT_3_TYPE(OUT_3_RESULT);
#endif
#if OUT_4
    OUT_4_CALC
    out4 = OUT_4_TYPE(OUT_4_RESULT);
#endif
#if OUT_5
    OUT_5_CALC
    out5 = OUT_5_TYPE(OUT_5_RESULT);
#endif
#if OUT_6
    OUT_6_CALC
    out6 = OUT_6_TYPE(OUT_6_RESULT);
#endif
#if OUT_7
    OUT_7_CALC
    out7 = OUT_7_TYPE(OUT_7_RESULT);
#endif
#if OUT_8
    OUT_8_CALC
    out8 = OUT_8_TYPE(OUT_8_RESULT);
#endif
#if OUT_9
    OUT_9_CALC
    out9 = OUT_9_TYPE(OUT_9_RESULT);
#endif
#if OUT_10
    OUT_10_CALC
    out10 = OUT_10_TYPE(OUT_10_RESULT);
#endif
#if OUT_11
    OUT_11_CALC
    out11 = OUT_11_TYPE(OUT_11_RESULT);
#endif
#if OUT_12
    OUT_12_CALC
    out12 = OUT_12_TYPE(OUT_12_RESULT);
#endif
#if OUT_13
    OUT_13_CALC
    out13 = OUT_13_TYPE(OUT_13_RESULT);
#endif
#if OUT_14
    OUT_14_CALC
    out14 = OUT_14_TYPE(OUT_14_RESULT);
#endif
#if OUT_15
    OUT_15_CALC
    out15 = OUT_15_TYPE(OUT_15_RESULT);
#endif
}
