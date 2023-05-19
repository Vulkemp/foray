#pragma once
#include "../core/core_declares.hpp"
#include "../core/managedresource.hpp"
#include "benchmarkbase.hpp"
#include <unordered_map>

namespace foray::bench {

    /// @brief A device benchmark based on Vulkans DeviceQuery Api. Timestamps are recorded in milliseconds. Timestamp names must be set in advance.
    class DeviceBenchmark : public BenchmarkBase, public core::ManagedResource
    {
      public:
        /// @brief Prepares object for operation
        /// @param context Requires DispatchTable, PhysicalDevice
        /// @param queryNames Must define all query names used (As they're mapped to integer Ids)
        /// @param uniqueSets count of unique sets
        DeviceBenchmark(core::Context* context, bool useIdSet, uint32_t queryCount = 64, uint32_t uniqueSets = INFLIGHT_FRAME_COUNT);

        virtual ~DeviceBenchmark();

        bool AttemptRetrieveResults(uint64_t frameIndex);

        /// @brief Resets the queries stored in current set indexes query pool
        /// @param cmdBuffer Command Buffer
        /// @param frameIndex Frame index for determining the query pool to access (frameIndex % uniqueSets)
        void CmdResetQuery(VkCommandBuffer cmdBuffer, uint64_t frameIndex);
        /// @brief Writes a timestamp to the current set indexes query pool
        /// @param cmdBuffer Command Buffer
        /// @param frameIndex Frame index for determining the query pool to access (frameIndex % uniqueSets)
        /// @param name Id name. Pointer must have been passed in the create method before!
        /// @param stageFlagBit Timestamp is written immediately before the pipelinestage defined here is invoked
        void CmdWriteBeginTimestamp(VkCommandBuffer cmdBuffer, uint64_t frameIndex, VkPipelineStageFlagBits stageFlagBit);
        /// @brief Writes a timestamp to the current set indexes query pool
        /// @param cmdBuffer Command Buffer
        /// @param frameIndex Frame index for determining the query pool to access (frameIndex % uniqueSets)
        /// @param name Id name. Pointer must have been passed in the create method before!
        /// @param stageFlagBit Timestamp is written immediately before the pipelinestage defined here is invoked
        void CmdWriteTimestamp(VkCommandBuffer cmdBuffer, uint64_t frameIndex, std::string_view name, VkPipelineStageFlagBits stageFlagBit);

      protected:
        fp64_t ConvertQueryResultToMillis(uint64_t result);

        struct QueryPoolSet
        {
            VkQueryPool                   Pool = nullptr;
            std::vector<std::string_view> RecordedIds;
        };

        core::Context*            mContext = nullptr;
        uint64_t                  mQueryCount;
        std::vector<QueryPoolSet> mSets;
        fp64_t                    mTimestampPeriod = 0.f;
        Local<util::StringSet>    mIdSet;
    };
}  // namespace foray::bench