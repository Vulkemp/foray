#ifdef BIND_SCENE_TRANSFORM_STATE
#ifndef SET_SCENE_TRANSFORM_STATE
#define SET_SCENE_TRANSFORM_STATE 0
#endif 

struct ModelTransformState
{
    mat4 ModelMatrix;
    mat4 PreviousModelMatrix;
};

layout (set = SET_SCENE_TRANSFORM_STATE, binding = BIND_SCENE_TRANSFORM_STATE) readonly buffer SceneTransformStateBuffer{ ModelTransformState Array[]; } SceneTransformState;

ModelTransformState GetTransformState(in int meshInstanceIndex) {
    return SceneTransformState.Array[meshInstanceIndex];
}

#endif