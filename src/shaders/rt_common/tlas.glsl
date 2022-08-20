/*
    rt_common/tlas.glsl

    Layout macro for binding the main Tlas
*/

#ifdef BIND_TLAS
#ifndef SET_TLAS
#define SET_TLAS 0
#endif  // SET_TLAS
/// @brief Main toplevel acceleration structure
layout(set = SET_TLAS, binding = BIND_TLAS) uniform accelerationStructureEXT MainTlas;
#endif  // BIND_TLAS
