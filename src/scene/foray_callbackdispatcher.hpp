#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../osi/foray_osi_declares.hpp"
#include "foray_component.hpp"
#include <vector>

namespace foray::scene {
    class CallbackDispatcher
    {
      public:
        friend Registry;

        virtual void InvokeUpdate(const base::FrameUpdateInfo& updateInfo);
        virtual void InvokeBeforeDraw(const base::FrameRenderInfo& renderInfo);
        virtual void InvokeDraw(SceneDrawInfo& renderInfo);
        virtual void InvokeOnEvent(const Event* event);
        virtual void InvokeOnResized(VkExtent2D event);

      protected:
        template <typename TCallback, typename TArg = typename TCallback::TArg>
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
        CallbackVector<Component::OnResizedCallback>  mOnResized  = {};
    };

    template <typename TCallback, typename TArg>
    void CallbackDispatcher::CallbackVector<TCallback, TArg>::Invoke(TArg arg)
    {
        for(auto callback : Listeners)
        {
            callback->Invoke(arg);
        }
    }

    template <typename TCallback, typename TArg>
    void CallbackDispatcher::CallbackVector<TCallback, TArg>::Add(TCallback* callback)
    {
        Listeners.push_back(callback);
    }

    template <typename TCallback, typename TArg>
    bool CallbackDispatcher::CallbackVector<TCallback, TArg>::Remove(TCallback* callback)
    {
        auto iter = std::find(Listeners.cbegin(), Listeners.cend(), callback);
        if(iter != Listeners.cend())
        {
            Listeners.erase(iter);
            return true;
        }
        return false;
    }

}  // namespace foray