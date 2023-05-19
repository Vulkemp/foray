#pragma once
#include "../base/framerenderinfo.hpp"
#include "../event.hpp"
#include "../logger.hpp"
#include "../osi/osi_declares.hpp"
#include "component.hpp"
#include <map>
#include <vector>

namespace foray::scene {

    /// @brief Type maintaining callback lists for event distribution
    class CallbackDispatcher
    {
      public:
        friend Registry;

        virtual void InvokeUpdate(TUpdateMessage updateInfo);
        virtual void InvokeDraw(TDrawMessage renderInfo);
        virtual void InvokeOnEvent(TOsEventMessage event);

        FORAY_PRIORITYDELEGATE(TUpdateMessage, Update)
        FORAY_DELEGATE(TDrawMessage, Draw)
        FORAY_DELEGATE(TOsEventMessage, OsEvent)
    };

}  // namespace foray::scene