#pragma once
#include "../hsk_basics.hpp"
#include "../hsk_exception.hpp"
#include "hsk_component.hpp"
// #include "hsk_rootregistry.hpp"
#include <vector>

namespace hsk {
    /// @brief Manages a type identified list of components
    /// @remark Only one component per type may be registered
    /// @remark This class manages lifetime of the attached components
    class Registry : public NoMoveDefaults
    {
      public:
        inline Registry() {}
        inline Registry(RootRegistry* root) : mRootRegistry(root) {}

        /// @brief Instantiates a new componente
        template <typename TComponent, typename... Args>
        inline TComponent* MakeComponent(Args&&... args);

        /// @brief Adds a manually instantiated component instance (initiate with new, Registry manages finalization)
        template <typename TComponent>
        inline void AddComponent(TComponent* component);

        /// @brief Moves a component registered to a different Registry to this
        template <typename TComponent>
        inline void MoveComponent(TComponent* component);

        /// @brief Test wether a component matching type TComponent is registered
        template <typename TComponent>
        inline bool HasComponent() const;

        /// @brief 
        template <typename TComponent>
        inline TComponent* GetComponent();

        template <typename TComponent>
        inline const TComponent* GetComponent() const;

        inline bool RemoveDeleteComponent(Component* component);

        virtual void Cleanup();

        inline virtual ~Registry() {}

        HSK_PROPERTY_GET(RootRegistry)
        HSK_PROPERTY_CGET(RootRegistry)

        inline Registry& SetRootRegistry(RootRegistry* rootRegistry);

        HSK_PROPERTY_GET(Components)
        HSK_PROPERTY_CGET(Components)

      protected:
        RootRegistry*           mRootRegistry = nullptr;
        std::vector<Component*> mComponents   = {};

        void Register(Component* component);
        bool Unregister(Component* component);

        void RegisterToRoot(Component* component);
        void UnregisterFromRoot(Component* component);
    };

    template <typename TComponent, typename... Args>
    inline TComponent* Registry::MakeComponent(Args&&... args){
        Assert(mRootRegistry, "Registry::AddComponent: No Root Registry defined!");
        Assert(!HasComponent<TComponent>(), "Registry::AddComponent: Already has component of same type attached!");

        TComponent* value = new TComponent(std::forward<Args>(args)...);
        Register(value);
        return value;
    }

    template <typename TComponent>
    inline void Registry::AddComponent(TComponent* component)
    {
        Assert(mRootRegistry, "Registry::AddComponent: No Root Registry defined!");
        Assert(component, "Registry::AddComponent: Parameter component is nullptr!");
        Assert(!HasComponent<TComponent>(), "Registry::AddComponent: Already has component of same type attached!");
        Assert(!component->GetRegistry(), "Registry::AddComponent: Component is already attached to other registry!");

        Register(component);
    }

    template <typename TComponent>
    inline void Registry::MoveComponent(TComponent* component)
    {
        Assert(mRootRegistry, "Registry::AddComponent: No Root Registry defined!");
        Assert(component, "Registry::AddComponent: Parameter component is nullptr!");
        Assert(!HasComponent<TComponent>(), "Registry::AddComponent: Already has component of same type attached!");
        if(component->GetRegistry())
        {
            Registry* registry = component->GetRegistry();
            registry->Unregister(component);
        }
        Register(component);
    }

    template <typename TComponent>
    inline bool Registry::HasComponent() const
    {
        const auto value = GetComponent<TComponent>();
        return value != nullptr;
    }

    template <typename TComponent>
    inline TComponent* Registry::GetComponent()
    {
        for(Component* component : mComponents)
        {
            auto cast = dynamic_cast<TComponent*>(component);
            if(cast)
            {
                return cast;
            }
        }
        return nullptr;
    }

    template <typename TComponent>
    inline const TComponent* Registry::GetComponent() const
    {
        for(const Component* component : mComponents)
        {
            auto cast = dynamic_cast<const TComponent*>(component);
            if(cast)
            {
                return cast;
            }
        }
        return nullptr;
    }

    inline bool Registry::RemoveDeleteComponent(Component* component)
    {
        Assert(component, "Registry::RemoveDeleteComponent: Parameter component is nullptr!");
        Assert(mRootRegistry, "Registry::RemoveDeleteComponent: No Root Registry defined!");
        Assert(Unregister(component), "Registry::RemoveDeleteComponent: Component not registered!");

        delete component;

        return false;
    }

    inline Registry& Registry::SetRootRegistry(RootRegistry* rootRegistry){
        Assert(mComponents.size() == 0, "Registry::SetRootRegistry: Cannot transfer root registry with components. Finalize first!");
        mRootRegistry = rootRegistry;
        return *this;
    }
}  // namespace hsk