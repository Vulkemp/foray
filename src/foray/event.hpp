#pragma once
#include <functional>
#include <map>
#include <unordered_set>

namespace foray::event {

#pragma region Delegate
    template <class TMessage>
    class Delegate;

    /// @brief A receiver is an object which manages a subscription to a delegation list and invocation of a message handler
    /// @tparam TMessage The message type passed to invocation methods. Consider a reference depending on complexity.
    /// @details If both mDelegate and mHandler are set, the event is hooked automatically
    template <class TMessage>
    class Receiver : public NoMoveDefaults
    {
        using TDelegate = Delegate<TMessage>;
        friend Delegate<TMessage>;

      public:
        using THandler = std::function<void(TMessage msg)>;

        inline Receiver(Delegate<TMessage>* delegate = nullptr, THandler handler = nullptr) : mDelegate(delegate), mHandler(handler) { Hook(); }

        inline ~Receiver() { Unhook(); }

        Receiver<TMessage>& Set(Delegate<TMessage>* delegate, THandler handler)
        {
            Unhook();
            mDelegate = delegate;
            mHandler  = handler;
            Hook();
            return *this;
        }

        Receiver<TMessage>& SetDelegate(Delegate<TMessage>* delegate)
        {
            if(mDelegate != delegate)
            {
                Unhook();
                mDelegate = delegate;
                Hook();
            }
            return *this;
        }
        Receiver<TMessage>& SetHandler(Delegate<TMessage>* handler)
        {
            Unhook();
            mHandler = handler;
            Hook();
            return *this;
        }

        /// @brief Remove the subscription and reset members
        inline void Destroy()
        {
            Unhook();
            mHandler  = nullptr;
            mDelegate = nullptr;
        }

      private:
        inline void         Hook();
        inline void         Unhook();
        inline void Invoke(TMessage message) { mHandler(message); }

        Delegate<TMessage>* mDelegate;
        THandler            mHandler;
    };

    /// @brief A delegate manages a list of Receiver instances subscribed to its event. The order of invocation is undefined.
    /// @tparam TMessage The message type passed to invocation methods. Consider a reference depending on complexity.
    template <class TMessage>
    class Delegate : public NoMoveDefaults
    {
        friend Receiver<TMessage>;
        using TReceiver = Receiver<TMessage>;

      public:
        /// @brief Unsubscribe all Receivers
        void Clear();

        /// @brief Invoke the callback on all receivers (undefined order)
        /// @param message Message object
        void Invoke(TMessage message);

      protected:
        void AddCallback(TReceiver& callback);
        void RemoveCallback(TReceiver& callback);

        using TEntrySet = std::unordered_set<TReceiver*>;
        TEntrySet mReceivers;
    };

    template <class TMessage>
    void Receiver<TMessage>::Hook()
    {
        if(mDelegate && mHandler)
        {
            mDelegate->AddCallback(*this);
        }
    }

    template <class TMessage>
    void Receiver<TMessage>::Unhook()
    {
        if(mDelegate && mHandler)
        {
            mDelegate->RemoveCallback(*this);
        }
    }

    template <class TMessage>
    void Delegate<TMessage>::AddCallback(TReceiver& receiver)
    {
        mReceivers.emplace(&receiver);
    }
    template <class TMessage>
    void Delegate<TMessage>::RemoveCallback(TReceiver& receiver)
    {
        mReceivers.erase(&receiver);
    }

    template <class TMessage>
    void Delegate<TMessage>::Clear()
    {
        for(TReceiver& receiver : mReceivers)
        {
            receiver.Destroy();
        }
        mReceivers.clear();
    }

    template <class TMessage>
    void Delegate<TMessage>::Invoke(TMessage message)
    {
        for(TReceiver* receiver : mReceivers)
        {
            receiver->Invoke(message);
        }
    }

/// @brief Define both an accessor member function and a matching delegate field
#define FORAY_DELEGATE(TMessage, Name)                                                                                                                                             \
  public:                                                                                                                                                                          \
    using TDelegate##Name = foray::event::Delegate<TMessage>;                                                                                                                      \
    TDelegate##Name* On##Name()                                                                                                                                                    \
    {                                                                                                                                                                              \
        return &mOn##Name;                                                                                                                                                         \
    }                                                                                                                                                                              \
                                                                                                                                                                                   \
  protected:                                                                                                                                                                       \
    TDelegate##Name mOn##Name;

#pragma endregion
#pragma region PriorityDelegate

    template <class TMessage>
    class PriorityDelegate;

    /// @brief A priority receiver is an object which manages a subscription to a priority delegation list and invocation of a message handler
    /// @tparam TMessage The message type passed to invocation methods. Consider a reference depending on complexity.
    /// @details If both mDelegate and mHandler are set, the event is hooked automatically
    template <class TMessage>
    class PriorityReceiver : public NoMoveDefaults
    {
        using TPriorityDelegate = PriorityDelegate<TMessage>;
        friend PriorityDelegate<TMessage>;

      public:
        using THandler = std::function<void(TMessage msg)>;

        inline PriorityReceiver(PriorityDelegate<TMessage>* delegate = nullptr, THandler handler = nullptr, int32_t priority = 0)
            : mDelegate(delegate), mHandler(handler), mPriority(priority)
        {
            Hook();
        }

        inline ~PriorityReceiver() { Unhook(); }

        PriorityReceiver<TMessage>& Set(PriorityDelegate<TMessage>* delegate, THandler handler, int32_t priority)
        {
            Unhook();
            mDelegate = delegate;
            mHandler  = handler;
            mPriority = priority;
            Hook();
            return *this;
        }

        PriorityReceiver<TMessage>& SetDelegate(PriorityDelegate<TMessage>* delegate)
        {
            if(mDelegate != delegate)
            {
                Unhook();
                mDelegate = delegate;
                Hook();
            }
            return *this;
        }
        PriorityReceiver<TMessage>& SetHandler(PriorityDelegate<TMessage>* handler)
        {
            Unhook();
            mHandler = handler;
            Hook();
            return *this;
        }
        PriorityReceiver<TMessage>& SetPriority(int32_t priority)
        {
            if(mPriority != priority)
            {
                Unhook();
                mPriority = priority;
                Hook();
            }
            return *this;
        }

        int32_t GetPriority() const { return mPriority; }

        inline void Destroy()
        {
            Unhook();
            mDelegate = nullptr;
            mHandler  = nullptr;
            mPriority = 0;
        }

      private:
        inline void         Unhook();
        inline void         Hook();
        inline void Invoke(TMessage message) { mHandler(message); }

        PriorityDelegate<TMessage>* mDelegate;
        THandler                    mHandler;
        int32_t                     mPriority;
    };

    /// @brief A priority delegate manages a list of Receiver instances subscribed to its event. Receivers are invocated in order from negative to positive priority.
    /// @tparam TMessage The message type passed to invocation methods. Consider a reference depending on complexity.
    template <class TMessage>
    class PriorityDelegate : public NoMoveDefaults
    {
        friend PriorityReceiver<TMessage>;
        using TReceiver = PriorityReceiver<TMessage>;

      public:
        void Clear();

        void Invoke(TMessage message);

      protected:
        void AddCallback(TReceiver& callback);
        void RemoveCallback(TReceiver& callback);

        using TEntrySet    = std::unordered_set<TReceiver*>;
        using TPrioritySet = std::map<int32_t, TEntrySet>;
        TPrioritySet mReceivers;
    };

    template <class TMessage>
    void PriorityReceiver<TMessage>::Hook()
    {
        if(mDelegate && mHandler)
        {
            mDelegate->AddCallback(*this);
        }
    }

    template <class TMessage>
    void PriorityReceiver<TMessage>::Unhook()
    {
        if(mDelegate && mHandler)
        {
            mDelegate->RemoveCallback(*this);
        }
    }

    template <class TMessage>
    void PriorityDelegate<TMessage>::AddCallback(TReceiver& receiver)
    {
        mReceivers[receiver.mPriority].emplace(&receiver);
    }
    template <class TMessage>
    void PriorityDelegate<TMessage>::RemoveCallback(TReceiver& receiver)
    {
        auto& set = mReceivers[receiver.mPriority];
        set.erase(&receiver);
        if(set.empty())
        {
            mReceivers.erase(receiver.mPriority);
        }
    }

    template <class TMessage>
    void PriorityDelegate<TMessage>::Clear()
    {
        mReceivers.clear();
    }

    template <class TMessage>
    void PriorityDelegate<TMessage>::Invoke(TMessage message)
    {
        for(auto& kvp : mReceivers)
        {
            for(PriorityReceiver<TMessage>* receiver : kvp.second)
            {
                receiver->Invoke(message);
            }
        }
    }

/// @brief Define both an accessor member function and a matching priority delegate field
#define FORAY_PRIORITYDELEGATE(TMessage, Name)                                                                                                                                     \
  public:                                                                                                                                                                          \
    using TDelegate##Name = foray::event::PriorityDelegate<TMessage>;                                                                                                              \
    TDelegate##Name* On##Name()                                                                                                                                                    \
    {                                                                                                                                                                              \
        return &mOn##Name;                                                                                                                                                         \
    }                                                                                                                                                                              \
                                                                                                                                                                                   \
  protected:                                                                                                                                                                       \
    TDelegate##Name mOn##Name;

#pragma endregion

}  // namespace foray::event
