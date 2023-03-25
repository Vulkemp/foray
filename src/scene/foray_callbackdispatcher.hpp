#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../foray_event.hpp"
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

        virtual void InvokeUpdate(TUpdateMessage updateInfo);
        virtual void InvokeDraw(TDrawMessage renderInfo);
        virtual void InvokeOnEvent(TOsEventMessage event);

        FORAY_PRIORITYDELEGATE(TUpdateMessage, Update)
        FORAY_DELEGATE(TDrawMessage, Draw)
        FORAY_DELEGATE(TOsEventMessage, OsEvent)
    };

}  // namespace foray::scene