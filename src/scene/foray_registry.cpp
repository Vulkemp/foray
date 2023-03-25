#include "foray_registry.hpp"
#include "foray_callbackdispatcher.hpp"

namespace foray::scene {

    void Registry::Register(Component* component)
    {
        mComponents.push_back(component);
        RegisterToRoot(component);
        component->mRegistry = this;
    }

    bool Registry::Unregister(Component* component)
    {
        for(auto iter = mComponents.cbegin(); iter != mComponents.cend(); ++iter)
        {
            if(component == *iter)
            {
                mComponents.erase(iter);
                UnregisterFromRoot(component);
                component->mRegistry = nullptr;
                return true;
            }
        }
        return false;
    }

    void Registry::RegisterToRoot(Component* component)
    {
        Component::DrawCallback* drawable = dynamic_cast<Component::DrawCallback*>(component);
        if(drawable)
        {
            drawable->SetDelegate(mCallbackDispatcher->OnDraw());
        }
        Component::UpdateCallback* updatable = dynamic_cast<Component::UpdateCallback*>(component);
        if(updatable)
        {
            updatable->SetDelegate(mCallbackDispatcher->OnUpdate());
        }
        Component::OnEventCallback* receiver = dynamic_cast<Component::OnEventCallback*>(component);
        if(receiver)
        {
            receiver->SetDelegate(mCallbackDispatcher->OnOsEvent());
        }
    }
    void Registry::UnregisterFromRoot(Component* component)
    {
        Component::DrawCallback* drawable = dynamic_cast<Component::DrawCallback*>(component);
        if(drawable)
        {
            drawable->SetDelegate(nullptr);
        }
        Component::UpdateCallback* updatable = dynamic_cast<Component::UpdateCallback*>(component);
        if(updatable)
        {
            updatable->SetDelegate(nullptr);
        }
        Component::OnEventCallback* receiver = dynamic_cast<Component::OnEventCallback*>(component);
        if(receiver)
        {
            receiver->SetDelegate(nullptr);
        }
    }

    void Registry::Destroy()
    {
        for(auto component : mComponents)
        {
            UnregisterFromRoot(component);
            delete component;
        }
        mComponents.resize(0);
    }

}  // namespace foray