#pragma once
#include "hsk_scenegraph_declares.hpp"

namespace {
    class SceneResource
    {
      public:
        inline virtual void OnSceneStructureChanged() {}
        inline virtual void OnSceneNodesMoved() {}
    };
}  // namespace