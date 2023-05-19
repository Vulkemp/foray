#pragma once
#include "context.hpp"
#include "descriptorbinding.hpp"
#include <unordered_map>

namespace foray::core {

    using DescriptorTypeCountMap = std::unordered_map<VkDescriptorType, uint32_t>;
    using DescriptorBindingMap   = std::unordered_map<const DescriptorBindingBase*, uint32_t>;

    /// @brief Wrapper managing the lifetime of a descriptorset layout
    class DescriptorLayout
    {
      public:
        class Builder
        {
          public:
            Builder& AddBinding(DescriptorBindingBase* binding);
            Builder& SetBinding(uint32_t index, DescriptorBindingBase* binding);

            FORAY_PROPERTY_V(PNext)
            FORAY_PROPERTY_V(Flags)
            FORAY_PROPERTY_R(Bindings)

          protected:
            void*                               mPNext = nullptr;
            VkDescriptorSetLayoutCreateFlags    mFlags = 0;
            std::vector<DescriptorBindingBase*> mBindings;
        };

        DescriptorLayout(Context* context, const Builder& builder);

        virtual ~DescriptorLayout();

        FORAY_GETTER_V(Context)
        FORAY_GETTER_V(Layout)
        FORAY_GETTER_CR(PerSetRequirements)
        FORAY_GETTER_CR(BindObjToBinding)

      protected:
        Context*               mContext;
        VkDescriptorSetLayout  mLayout;
        DescriptorTypeCountMap mPerSetRequirements;
        DescriptorBindingMap   mBindObjToBinding;
    };

    /// @brief Wrapper managing the lifetime of a descriptor pool
    class DescriptorPool
    {
      public:
        class Builder
        {
          public:
            Builder& AddSets(const DescriptorLayout* layout, uint32_t count = 1);

            FORAY_PROPERTY_V(PNext)
            FORAY_PROPERTY_V(Flags)
            FORAY_PROPERTY_V(MaxSets)
            FORAY_PROPERTY_R(TypeCounts)
          protected:
            void*                       mPNext   = nullptr;
            VkDescriptorPoolCreateFlags mFlags   = 0;
            uint32_t                    mMaxSets = 0;
            DescriptorTypeCountMap      mTypeCounts;
        };

        /// @brief The UBER pool
        /// @remarks
        /// COMBINED_IMAGE_SAMPLER     : 256
        /// STORAGE_IMAGE              : 128
        /// UNIFORM_BUFFER             : 64
        /// STORAGE_BUFFER             : 64
        /// ACCELERATION_STRUCTURE_KHR : 16
        DescriptorPool(core::Context* context);
        DescriptorPool(core::Context* context, const Builder& builder);
        virtual ~DescriptorPool();

        FORAY_GETTER_V(Context)
        FORAY_GETTER_V(Pool)
        FORAY_GETTER_V(AllowFree)

      protected:
        Context*         mContext;
        VkDescriptorPool mPool;
        bool             mAllowFree;
    };

    /// @brief Wrapper managing lifetime and state of a descriptor set
    class DescriptorSet
    {
      public:
        class Builder
        {
          public:
            FORAY_PROPERTY_V(PNext)
            FORAY_PROPERTY_V(Pool)
            FORAY_PROPERTY_V(Layout)
          protected:
            void*             mPNext  = nullptr;
            DescriptorPool*   mPool   = nullptr;
            DescriptorLayout* mLayout = nullptr;
        };

        DescriptorSet(Context* context, const Builder& builder);
        virtual ~DescriptorSet();

        /// @brief Update information
        /// @remark If adding VkWriteDescriptorSet instead of binding object refs, .dstBinding must be set!
        class UpdateInfo
        {
          public:
            UpdateInfo& AddWrite(const VkWriteDescriptorSet& write);
            UpdateInfo& AddWrite(const core::DescriptorBindingBase* binding);

            FORAY_PROPERTY_R(BindingWrites)
            FORAY_PROPERTY_R(VkWrites)
          protected:
            std::vector<const core::DescriptorBindingBase*> mBindingWrites;
            std::vector<VkWriteDescriptorSet>               mVkWrites;
        };

        void Update(const UpdateInfo& update);

        FORAY_GETTER_V(Context)
        FORAY_GETTER_V(Layout)
        FORAY_GETTER_V(Set)

      protected:
        Context*             mContext;
        DescriptorPool*      mPool;
        DescriptorLayout*    mLayout;
        VkDescriptorSet      mSet;
        DescriptorBindingMap mBindObjToBinding;
    };

}  // namespace foray::core