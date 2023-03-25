#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../foray_logger.hpp"
#include "../osi/foray_osi_declares.hpp"
#include "foray_component.hpp"
#include <map>
#include <vector>

namespace foray::scene {
    /// @brief Type maintaining callback lists for event distribution
    class CallbackDispatcher
    {
      public:
        friend Registry;

        virtual void InvokeUpdate(SceneUpdateInfo& updateInfo);
        virtual void InvokeDraw(SceneDrawInfo& renderInfo);
        virtual void InvokeOnEvent(const osi::Event* event);

      protected:
        template <typename TCallback, bool Ordered = TCallback::ORDERED_EXECUTION>
        struct CallbackVector
        {
            std::vector<TCallback*> Listeners = {};

            inline void Invoke(typename TCallback::TArg arg);
            inline void Add(TCallback* callback);
            inline bool Remove(TCallback* callback);
        };

        template <typename TCallback>
        struct CallbackVector<TCallback, true>
        {
            std::multimap<int32_t, TCallback*> Listeners = {};

            inline void Invoke(typename TCallback::TArg arg);
            inline void Add(TCallback* callback);
            inline bool Remove(TCallback* callback);
        };

        CallbackVector<Component::OnEventCallback>   mOnEvent   = {};
        CallbackVector<Component::UpdateCallback>    mUpdate    = {};
        CallbackVector<Component::DrawCallback>      mDraw      = {};
    };

    template <typename TCallback, bool Ordered>
    void CallbackDispatcher::CallbackVector<TCallback, Ordered>::Invoke(typename TCallback::TArg arg)
    {
        for(auto callback : Listeners)
        {
            callback->Invoke(arg);
        }
    }

    template <typename TCallback, bool Ordered>
    void CallbackDispatcher::CallbackVector<TCallback, Ordered>::Add(TCallback* callback)
    {
        Listeners.push_back(callback);
    }

    template <typename TCallback, bool Ordered>
    bool CallbackDispatcher::CallbackVector<TCallback, Ordered>::Remove(TCallback* callback)
    {
        auto iter = std::find(Listeners.cbegin(), Listeners.cend(), callback);
        if(iter != Listeners.cend())
        {
            Listeners.erase(iter);
            return true;
        }
        return false;
    }

    template <typename TCallback>
    void CallbackDispatcher::CallbackVector<TCallback, true>::Invoke(typename TCallback::TArg arg)
    {
        for(auto callback : Listeners)
        {
            callback.second->Invoke(arg);
        }
    }

    template <typename TCallback>
    void CallbackDispatcher::CallbackVector<TCallback, true>::Add(TCallback* callback)
    {
        Listeners.emplace(callback->GetOrder(), callback);
    }

    template <typename TCallback>
    bool CallbackDispatcher::CallbackVector<TCallback, true>::Remove(TCallback* callback)
    {
        auto range = Listeners.equal_range(callback->GetOrder());
        for(auto iter = range.first; iter != range.second; ++iter)
        {
            if(iter->second == callback)
            {
                Listeners.erase(iter);
                return true;
            }
        }
        return false;
    }

}  // namespace foray::scene