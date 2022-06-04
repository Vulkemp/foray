#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../hsk_basics.hpp"
#include "../osi/hsk_osi_declares.hpp"
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
            inline virtual void Update(const FrameUpdateInfo& updateInfo) {}
            inline void         Invoke(const FrameUpdateInfo& updateInfo) { Update(updateInfo); }
        };

        class BeforeDrawCallback : public Polymorphic
        {
          public:
            using TArg = const FrameRenderInfo&;
            inline virtual void BeforeDraw(const FrameRenderInfo& renderInfo) {}
            inline void         Invoke(const FrameRenderInfo& renderInfo) { BeforeDraw(renderInfo); }
        };

        class DrawCallback : public Polymorphic
        {
          public:
            using TArg = const FrameRenderInfo&;
            inline virtual void Draw(const FrameRenderInfo& renderInfo) {}
            inline void         Invoke(const FrameRenderInfo& renderInfo) { Draw(renderInfo); }
        };

        class OnEventCallback : public Polymorphic
        {
          public:
            using TArg = std::shared_ptr<Event>&;
            inline virtual void OnEvent(std::shared_ptr<Event>& event) {}
            inline void         Invoke(std::shared_ptr<Event>& event) { OnEvent(event); }
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