#pragma once
#include "hsk_benchmarkbase.hpp"
#include <unordered_map>

namespace hsk {

    struct VkContext;

    class DeviceBenchmark : public BenchmarkBase, public DeviceResourceBase
    {
      public:
        void Create(const VkContext* context, const std::vector<const char*>& queryNames, uint32_t uniqueSets = INFLIGHT_FRAME_COUNT);

        inline virtual bool Exists() const { return mQueryPools.size() > 0; }
        virtual void        Destroy();

        void CmdResetQuery(VkCommandBuffer cmdBuffer, uint32_t frameIndex);
        void CmdWriteTimestamp(VkCommandBuffer cmdBuffer, uint32_t frameIndex, const char* name, VkPipelineStageFlagBits stageFlagBit);

        bool LogQueryResults(uint32_t frameIndex);

        inline virtual ~DeviceBenchmark() { Destroy(); }

      protected:
        fp64_t ConvertQueryResultToMillis(uint64_t result);

        const VkContext*         mContext = nullptr;
        std::vector<VkQueryPool> mQueryPools;
        fp64_t                   mTimestampPeriod = 0.f;

        std::vector<const char*>                  mQueryNames;
        std::unordered_map<const char*, uint32_t> mQueryIds;
    };
}  // namespace hsk