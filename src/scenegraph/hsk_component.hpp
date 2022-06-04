#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../hsk_basics.hpp"
#include "../osi/hsk_osi_declares.hpp"
#include "hsk_scenedrawing.hpp"
#include "hsk_scenegraph_declares.hpp"

namespace hsk {

    class Component : public NoMoveDefaults, public Polymorphic
    {
      public:
        friend Registry;

        class UpdateCallback : public Polymorphic
        {
          public:
            using TArg = const FrameUpdateInfo&;
            inline virtual void Update(TArg updateInfo) = 0;
            inline void         Invoke(TArg updateInfo) { Update(updateInfo); }
        };

        class BeforeDrawCallback : public Polymorphic
        {
          public:
            using TArg = const FrameRenderInfo&;
            inline virtual void BeforeDraw(TArg renderInfo) = 0;
            inline void         Invoke(TArg renderInfo) { BeforeDraw(renderInfo); }
        };

        class DrawCallback : public Polymorphic
        {
          public:
            using TArg = SceneDrawInfo&;
            inline virtual void Draw(TArg drawInfo) = 0;
            inline void         Invoke(TArg drawInfo) { Draw(drawInfo); }
        };

        class OnEventCallback : public Polymorphic
        {
          public:
            using TArg = std::shared_ptr<Event>&;
            inline virtual void OnEvent(TArg event) = 0;
            inline void         Invoke(TArg event) { OnEvent(event); }
        };

        inline virtual ~Component() {}

        HSK_PROPERTY_CGET(Registry)
        HSK_PROPERTY_GET(Registry)

        NNode*  GetNode();
        NScene* GetScene();

      private:
        Registry* mRegistry = nullptr;
    };
}  // namespace hsk