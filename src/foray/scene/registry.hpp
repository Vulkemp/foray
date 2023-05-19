#pragma once
#include "../basics.hpp"
#include "../exception.hpp"
#include "component.hpp"
// #include "rootregistry.hpp"
#include <vector>

namespace foray::scene {
    /// @brief Manages a type identified list of components
    /// @remark This class manages lifetime of the attached components
    class Registry : public NoMoveDefaults
    {
      public:
        inline Registry() {}
        inline Registry(CallbackDispatcher* root) : mCallbackDispatcher(root) {}

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

        /// @brief Gets first component that can be cast to TComponent type
        template <typename TComponent>
        inline TComponent* GetComponent();

        /// @brief Gets first component that can be cast to TComponent type
        template <typename TComponent>
        inline const TComponent* GetComponent() const;

        /// @brief Appends all components which can be cast to TComponent type to the out vector
        template <typename TComponent>
        inline int32_t GetComponents(std::vector<TComponent*>& out);

        /// @brief Appends all components which can be cast to TComponent type to the out vector
        template <typename TComponent>
        inline int32_t GetComponents(std::vector<const TComponent*>& out) const;

        /// @brief Removes and finalizes a component
        inline bool RemoveDeleteComponent(Component* component);

        /// @brief Finalizes all attached components
        virtual void Destroy();

        inline virtual ~Registry() { Destroy(); }

        /// @brief The root registry manages global callbacks invokable on the components
        FORAY_GETTER_V(CallbackDispatcher)

        inline Registry& SetCallbackDispatcher(CallbackDispatcher* rootRegistry);

        /// @brief All components attached to the registry
        FORAY_GETTER_CR(Components)

      protected:
        CallbackDispatcher*     mCallbackDispatcher = nullptr;
        std::vector<Component*> mComponents         = {};

        void Register(Component* component);
        bool Unregister(Component* component);

        void RegisterToRoot(Component* component);
        void UnregisterFromRoot(Component* component);
    };

    template <typename TComponent, typename... Args>
    inline TComponent* Registry::MakeComponent(Args&&... args)
    {
        Assert(mCallbackDispatcher, "Registry::AddComponent: No Root Registry defined!");

        TComponent* value = new TComponent(std::forward<Args>(args)...);
        Register(value);
        return value;
    }

    template <typename TComponent>
    inline void Registry::AddComponent(TComponent* component)
    {
        Assert(mCallbackDispatcher, "Registry::AddComponent: No Root Registry defined!");
        Assert(component, "Registry::AddComponent: Parameter component is nullptr!");
        Assert(!component->GetRegistry(), "Registry::AddComponent: Component is already attached to other registry!");

        Register(component);
    }

    template <typename TComponent>
    inline void Registry::MoveComponent(TComponent* component)
    {
        Assert(mCallbackDispatcher, "Registry::AddComponent: No Root Registry defined!");
        Assert(component, "Registry::AddComponent: Parameter component is nullptr!");
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

    template <typename TComponent>
    inline int32_t Registry::GetComponents(std::vector<TComponent*>& out)
    {
        int32_t writes = 0;
        for(Component* component : mComponents)
        {
            auto cast = dynamic_cast<TComponent*>(component);
            if(cast)
            {
                out.push_back(cast);
                writes++;
            }
        }
        return writes;
    }

    template <typename TComponent>
    inline int32_t Registry::GetComponents(std::vector<const TComponent*>& out) const
    {
        int32_t writes = 0;
        for(const Component* component : mComponents)
        {
            auto cast = dynamic_cast<const TComponent*>(component);
            if(cast)
            {
                out.push_back(cast);
                writes++;
            }
        }
        return writes;
    }

    inline bool Registry::RemoveDeleteComponent(Component* component)
    {
        Assert(component, "Registry::RemoveDeleteComponent: Parameter component is nullptr!");
        Assert(mCallbackDispatcher, "Registry::RemoveDeleteComponent: No Root Registry defined!");
        Assert(Unregister(component), "Registry::RemoveDeleteComponent: Component not registered!");

        delete component;

        return false;
    }

    inline Registry& Registry::SetCallbackDispatcher(CallbackDispatcher* rootRegistry)
    {
        Assert(mComponents.size() == 0, "Registry::SetCallbackDispatcher: Cannot transfer root registry with components. Finalize first!");
        mCallbackDispatcher = rootRegistry;
        return *this;
    }
}  // namespace foray