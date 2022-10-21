#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include <unordered_set>
#include <spdlog/spdlog.h>

namespace foray::core {

    /// @brief Base class enforcing common interface for all classes wrapping a device resource
    class ManagedResource : public NoMoveDefaults
    {
        /// @brief Every allocation registers with its name, so when clearing up, we can track if there are unfreed resources left.
        static inline std::unordered_set<ManagedResource*> sAllocatedRessources{};

      public:
        static void sPrintAllocatedResources(bool printAsWarning);
        static const std::unordered_set<ManagedResource*>* GetTotalAllocatedResources() { return &sAllocatedRessources; }

        virtual std::string_view GetTypeName() const;
        virtual bool             Exists() const = 0;
        virtual void             Destroy()      = 0;

        ManagedResource();
        explicit ManagedResource(std::string_view name);
        virtual ~ManagedResource();

        std::string_view         GetName() const { return mName; }
        virtual void SetName(std::string_view name);

        std::string Print() const;

      protected:
        std::string mName;
    };

    std::string_view PrintVkObjectType(VkObjectType objecType);

    template <VkObjectType OBJECT_TYPE>
    class VulkanResource : public ManagedResource
    {
      public:
        inline VulkanResource() : ManagedResource() {}
        inline explicit VulkanResource(std::string_view name) : ManagedResource(name) {}

        inline virtual std::string_view GetTypeName() const;

      protected:
        inline virtual void SetObjectName(core::Context* context, const void* handle, std::string_view name, bool updateResourceName = true);
    };

    template <VkObjectType OBJECT_TYPE>
    inline std::string_view VulkanResource<OBJECT_TYPE>::GetTypeName() const
    {
        return PrintVkObjectType(OBJECT_TYPE);
    }

    template <VkObjectType OBJECT_TYPE>
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