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
            mCallbackDispatcher->mDraw.Add(drawable);
        }
        Component::UpdateCallback* updatable = dynamic_cast<Component::UpdateCallback*>(component);
        if(updatable)
        {
            mCallbackDispatcher->mUpdate.Add(updatable);
        }
        Component::OnEventCallback* receiver = dynamic_cast<Component::OnEventCallback*>(component);
        if(receiver)
        {
            mCallbackDispatcher->mOnEvent.Add(receiver);
        }
        Component::OnResizedCallback* resizable = dynamic_cast<Component::OnResizedCallback*>(component);
        if(resizable)
        {
            mCallbackDispatcher->mOnResized.Add(resizable);
        }
    }
    void Registry::UnregisterFromRoot(Component* component)
    {
        Component::DrawCallback* drawable = dynamic_cast<Component::DrawCallback*>(component);
        if(drawable)
        {
            mCallbackDispatcher->mDraw.Remove(drawable);
        }
        Component::UpdateCallback* updatable = dynamic_cast<Component::UpdateCallback*>(component);
        if(updatable)
        {
            mCallbackDispatcher->mUpdate.Remove(updatable);
        }
        Component::OnEventCallback* receiver = dynamic_cast<Component::OnEventCallback*>(component);
        if(receiver)
        {
            mCallbackDispatcher->mOnEvent.Remove(receiver);
        }
        Component::OnResizedCallback* resizable = dynamic_cast<Component::OnResizedCallback*>(component);
        if(resizable)
        {
            mCallbackDispatcher->mOnResized.Remove(resizable);
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