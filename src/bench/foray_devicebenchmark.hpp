#pragma once
#include "../core/foray_managedresource.hpp"
#include "../core/foray_core_declares.hpp"
#include "foray_benchmarkbase.hpp"
#include <unordered_map>

namespace foray::bench {

    struct VkContext;

    /// @brief A device benchmark based on Vulkans DeviceQuery Api
    class DeviceBenchmark : public BenchmarkBase, public core::ManagedResource
    {
      public:
        /// @brief Prepares object for operation
        /// @param context Requires DispatchTable, PhysicalDevice
        /// @param queryNames Must define all query names used (As they're mapped to integer Ids)
        /// @param uniqueSets count of unique sets
        void Create(core::Context* context, const std::vector<const char*>& queryNames, uint32_t uniqueSets = INFLIGHT_FRAME_COUNT);

        inline virtual bool Exists() const { return mQueryPools.size() > 0; }
        virtual void        Destroy();

        /// @brief Resets the queries stored in current set indexes query pool
        /// @param cmdBuffer Command Buffer
        /// @param frameIndex Frame index for determining the query pool to access (frameIndex % uniqueSets)
        void CmdResetQuery(VkCommandBuffer cmdBuffer, uint64_t frameIndex);
        /// @brief Writes a timestamp to the current set indexes query pool
        /// @param cmdBuffer Command Buffer
        /// @param frameIndex Frame index for determining the query pool to access (frameIndex % uniqueSets)
        /// @param name Id name. Pointer must have been passed in the create method before!
        /// @param stageFlagBit Timestamp is written immediately before the pipelinestage defined here is invoked
        void CmdWriteTimestamp(VkCommandBuffer cmdBuffer, uint64_t frameIndex, const char* name, VkPipelineStageFlagBits stageFlagBit);

        /// @brief Instructs the benchmark object to retrieve the query results for a frame
        /// @param frameIndex Frame index for determining the query pool to access (frameIndex % uniqueSets)
        /// @return True, if log operation succeeds
        bool LogQueryResults(uint64_t frameIndex);

        inline virtual ~DeviceBenchmark() { Destroy(); }

      protected:
        fp64_t ConvertQueryResultToMillis(uint64_t result);

        core::Context*         mContext = nullptr;
        std::vector<VkQueryPool> mQueryPools;
        fp64_t                   mTimestampPeriod = 0.f;

        std::vector<const char*>                  mQueryNames;
        std::unordered_map<const char*, uint32_t> mQueryIds;
    };
}  // namespace foray::bench