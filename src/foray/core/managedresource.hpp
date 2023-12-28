#pragma once
#include "../basics.hpp"
#include "../vulkan.hpp"
#include <unordered_set>
#include <spdlog/spdlog.h>

namespace foray::core {

    /// @brief Base class enforcing common interface for all classes wrapping a device resource
    class ManagedResource : public NoMoveDefaults
    {
        /// @brief Every allocation registers with its name, so when clearing up, we can track if there are unfreed resources left.
        static inline std::unordered_set<ManagedResource*> sAllocatedRessources{};

      public:
        /// @brief Print a list of all registered existing resources
        /// @param printAsWarning If true, a non-zero amount is logged as a warning
        static void sPrintAllocatedResources(bool printAsWarning);
        static const std::unordered_set<ManagedResource*>* GetTotalAllocatedResources() { return &sAllocatedRessources; }

        /// @brief Return a hint for the type of resource managed by the instantiation
        /// @return 
        virtual std::string_view GetTypeName() const;
        /// @brief Return true, if the managed resource is allocated
        /// @details Example: Vulkan object stored could be nullptr or instantiated
        virtual bool             Exists() const {return true;}

        /// @brief Default constructor registers the resource
        ManagedResource();
        /// @brief Registers the resource and sets its name
        explicit ManagedResource(std::string_view name);
        /// @brief Unregisters the resource
        virtual ~ManagedResource();

        /// @brief Return a custom name for the object
        std::string_view         GetName() const { return mName; }
        /// @brief Set a custom name for the object
        virtual void SetName(std::string_view name);

        /// @brief Print name and type in one string
        std::string Print() const;

      protected:
        /// @brief This objects custom name
        std::string mName;
    };

    /// @brief Uses nameof.hpp's NAMEOF_ENUM function to stringify vk::ObjectType
    std::string_view PrintVkObjectType(vk::ObjectType objecType);

    /// @brief ManagedResource variant which automates GetTypeName() overloading by returning a stringified version of vk::ObjectType
    /// @tparam OBJECT_TYPE Object type managed by the inheriting class
    template <vk::ObjectType OBJECT_TYPE>
    class VulkanResource : public ManagedResource
    {
      public:
        inline VulkanResource() : ManagedResource() {}
        inline explicit VulkanResource(std::string_view name) : ManagedResource(name) {}

        /// @brief Returns <OBJECT_TYPE> stringified
        inline virtual std::string_view GetTypeName() const;

      protected:
        /// @brief Set the object name. Sets both ManagedResource::mName aswell as vulkan debug object name
        /// @param context Requires DispatchTable
        /// @param handle Object handle
        /// @param name Name
        /// @param updateResourceName If true, sets ManagedResource::mName
        inline virtual void SetObjectName(core::Context* context, const void* handle, std::string_view name, bool updateResourceName = true);
    };

    template <vk::ObjectType OBJECT_TYPE>
    inline std::string_view VulkanResource<OBJECT_TYPE>::GetTypeName() const
    {
        return PrintVkObjectType(OBJECT_TYPE);
    }

    template <vk::ObjectType OBJECT_TYPE>
    inline void VulkanResource<OBJECT_TYPE>::SetObjectName(core::Context* context, const void* handle, std::string_view name, bool updateResourceName)
    {
#ifdef FORAY_DEBUG
        SetVulkanObjectName(context, OBJECT_TYPE, handle, name);
#endif
        if(updateResourceName)
        {
            ManagedResource::SetName(name);
        }
    }

}  // namespace foray::core