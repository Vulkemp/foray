#include "foray_devicebenchmark.hpp"
#include "../core/foray_context.hpp"

namespace foray::bench {
    DeviceBenchmark::DeviceBenchmark(core::Context* context, bool useIdSet, uint32_t queryCount, uint32_t uniqueSets)
        : mContext(context), mQueryCount(queryCount), mSets(uniqueSets)
    {
        mTimestampPeriod = (fp64_t)mContext->Device->GetPhysicalDevice().properties.limits.timestampPeriod;

        for(QueryPoolSet& set : mSets)
        {
            VkQueryPoolCreateInfo poolCi{.sType              = VkStructureType::VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                         .pNext              = nullptr,
                                         .flags              = 0,
                                         .queryType          = VkQueryType::VK_QUERY_TYPE_TIMESTAMP,
                                         .queryCount         = static_cast<uint32_t>(mQueryCount),
                                         .pipelineStatistics = 0};

            mContext->DispatchTable().createQueryPool(&poolCi, nullptr, &set.Pool);
        }

        if(useIdSet)
        {
            mIdSet.New();
        }
    }

    void DeviceBenchmark::CmdResetQuery(VkCommandBuffer cmdBuffer, uint64_t frameIndex)
    {
        QueryPoolSet& set = mSets[frameIndex % mSets.size()];
        set.RecordedIds.clear();
        mContext->DispatchTable().cmdResetQueryPool(cmdBuffer, set.Pool, 0, mQueryCount);
    }
    void DeviceBenchmark::CmdWriteBeginTimestamp(VkCommandBuffer cmdBuffer, uint64_t frameIndex, VkPipelineStageFlagBits stageFlagBit) 
    {
        static const char* BEGIN = "Begin";
        CmdWriteTimestamp(cmdBuffer, frameIndex, BEGIN, stageFlagBit);
    }
    void DeviceBenchmark::CmdWriteTimestamp(VkCommandBuffer cmdBuffer, uint64_t frameIndex, std::string_view name, VkPipelineStageFlagBits stageFlagBit)
    {
        QueryPoolSet& set = mSets[frameIndex % mSets.size()];
        mContext->DispatchTable().cmdWriteTimestamp(cmdBuffer, stageFlagBit, set.Pool, (uint32_t)set.RecordedIds.size());
        if(mIdSet)
        {
            name = mIdSet->Add(name);
        }
        set.RecordedIds.push_back(name);
    }

    struct QueryResult
    {
        uint64_t Timestamp;
        uint64_t Available;
    };

    bool DeviceBenchmark::AttemptRetrieveResults(uint64_t frameIndex)
    {
        std::vector<QueryResult> results(mQueryCount);
        QueryPoolSet&            set = mSets[frameIndex % mSets.size()];
        mContext->DispatchTable().getQueryPoolResults(set.Pool, 0, set.RecordedIds.size(), sizeof(QueryResult) * results.size(), results.data(), sizeof(QueryResult),
                                                      VkQueryResultFlagBits::VK_QUERY_RESULT_64_BIT | VkQueryResultFlagBits::VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        if(results[set.RecordedIds.size() - 1].Available == 0)
        {
            return false;
        }

        RepetitionLog recording;

        fp64_t start = ConvertQueryResultToMillis(results.front().Timestamp);

        for(int32_t id = 1; id < (int32_t)set.RecordedIds.size(); id++)
        {
            fp64_t delta = ConvertQueryResultToMillis(results[id].Timestamp) - start;
            recording.Append(nullptr, set.RecordedIds[id], delta);
        }

        mOnLogFinalized.Invoke(recording);

        return true;
    }


    fp64_t DeviceBenchmark::ConvertQueryResultToMillis(uint64_t result)
    {
        fp64_t nanos = result * mTimestampPeriod;
        return nanos / (1000.0 * 1000.0);
    }

    DeviceBenchmark::~DeviceBenchmark()
    {
        for(QueryPoolSet& set : mSets)
        {
            mContext->DispatchTable().destroyQueryPool(set.Pool, nullptr);
        }
    }
}  // namespace foray::bench