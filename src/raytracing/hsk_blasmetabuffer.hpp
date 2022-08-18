#pragma
#include "../memory/hsk_managedbuffer.hpp"
#include "hsk_blas.hpp"
#include <unordered_map>
#include <unordered_set>
#include "../memory/hsk_descriptorsethelper.hpp"

namespace hsk {
    struct BlasGeometryMeta
    {
        int32_t  MaterialIndex     = 0;
        uint32_t IndexBufferOffset = 0U;
    };

    class BlasMetaBuffer
    {
      public:
        const std::unordered_map<const Blas*, uint32_t>& CreateOrUpdate(const VkContext* context, const std::unordered_set<const Blas*>& entries);

        HSK_PROPERTY_ALLGET(Buffer)
        HSK_PROPERTY_CGET(BufferOffsets)

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetDescriptorInfo(VkShaderStageFlags shaderStage);

      protected:
        const VkContext*                    mContext = nullptr;
        std::unordered_map<const Blas*, uint32_t> mBufferOffsets;
        ManagedBuffer                       mBuffer;
        std::vector<VkDescriptorBufferInfo> mDescriptorInfos;
    };
}  // namespace hsk