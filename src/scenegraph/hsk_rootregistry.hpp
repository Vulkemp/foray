#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../osi/hsk_osi_declares.hpp"
#include "hsk_component.hpp"
#include <vector>

namespace hsk {
    class RootRegistry
    {
      public:
        friend Registry;

        virtual void InvokeUpdate(const FrameUpdateInfo& updateInfo);
        virtual void InvokeBeforeDraw(const FrameRenderInfo& renderInfo);
        virtual void InvokeDraw(SceneDrawInfo& renderInfo);
        virtual void InvokeOnEvent(std::shared_ptr<Event> event);

      protected:
        template <typename TCallback, typename TArg = TCallback::TArg>
        struct CallbackVector
        {
            std::vector<TCallback*> Listeners = {};

            inline void Invoke(TArg arg);
            inline void Add(TCallback* callback);
            inline bool Remove(TCallback* callback);
        };

        CallbackVector<Component::OnEventCallback>    mOnEvent    = {};
        CallbackVector<Component::UpdateCallback>     mUpdate     = {};
        CallbackVector<Component::DrawCallback>       mDraw       = {};
        CallbackVector<Component::BeforeDrawCallback> mBeforeDraw = {};
    };

    template <typename TCallback, typename TArg>
    void RootRegistry::CallbackVector<TCallback, TArg>::Invoke(TArg arg)
    {
        for(auto callback : Listeners)
        {
            callback->Invoke(arg);
        }
    }

    template <typename TCallback, typename TArg>
    void RootRegistry::CallbackVector<TCallback, TArg>::Add(TCallback* callback)
    {
        Listeners.push_back(callback);
    }

    template <typename TCallback, typename TArg>
    bool RootRegistry::CallbackVector<TCallback, TArg>::Remove(TCallback* callback)
    {
        auto iter = std::find(Listeners.cbegin(), Listeners.cend(), callback);
        if(iter != Listeners.cend())
        {
            Listeners.erase(iter);
            return true;
        }
        return false;
    }

}  // namespace hsk