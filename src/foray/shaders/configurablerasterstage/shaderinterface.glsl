#if INTERFACE_WORLDPOS
layout(location = 0) INTERFACEMODE vec3 WorldPos;
#endif
#if INTERFACE_WORLDPOSOLD
layout(location = 1) INTERFACEMODE vec3 WorldPosOld;
#endif
#if INTERFACE_DEVICEPOS
layout(location = 2) INTERFACEMODE vec4 DevicePos;
#endif
#if INTERFACE_DEVICEPOSOLD
layout(location = 3) INTERFACEMODE vec4 DevicePosOld;
#endif
#if INTERFACE_NORMAL
layout(location = 4) INTERFACEMODE vec3 Normal;
#endif
#if INTERFACE_TANGENT
layout(location = 5) INTERFACEMODE vec3 Tangent;
#endif
#if INTERFACE_UV
layout(location = 6) INTERFACEMODE vec2 UV;
#endif
#if INTERFACE_MESHID
layout(location = 7) flat INTERFACEMODE uint MeshInstanceId;
#endif
