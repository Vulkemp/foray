#pragma once
#include "../core/foray_deviceresource.hpp"
#include "../core/foray_core_declares.hpp"
#include "foray_benchmarkbase.hpp"
#include <unordered_map>

namespace foray::bench {

    struct VkContext;

    class DeviceBenchmark : public BenchmarkBase, public core::DeviceResourceBase
    {
      public:
        void Create(const core::VkContext* context, const std::vector<const char*>& queryNames, uint32_t uniqueSets = INFLIGHT_FRAME_COUNT);

        inline virtual bool Exists() const { return mQueryPools.size() > 0; }
        virtual void        Destroy();

        void CmdResetQuery(VkCommandBuffer cmdBuffer, uint64_t frameIndex);
        void CmdWriteTimestamp(VkCommandBuffer cmdBuffer, uint64_t frameIndex, const char* name, VkPipelineStageFlagBits stageFlagBit);

        bool LogQueryResults(uint64_t frameIndex);

        inline virtual ~DeviceBenchmark() { Destroy(); }

      protected:
        fp64_t ConvertQueryResultToMillis(uint64_t result);

        const core::VkContext*         mContext = nullptr;
        std::vector<VkQueryPool> mQueryPools;
        fp64_t                   mTimestampPeriod = 0.f;

        std::vector<const char*>                  mQueryNames;
        std::unordered_map<const char*, uint32_t> mQueryIds;
    };
}  // namespace foray::bench