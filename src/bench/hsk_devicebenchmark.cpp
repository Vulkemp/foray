#include "hsk_devicebenchmark.hpp"
#include "../base/hsk_vkcontext.hpp"

namespace hsk {
    void DeviceBenchmark::Create(const VkContext* context, const std::vector<const char*>& queryNames, uint32_t uniqueSets)
    {
        mContext = context;

        mTimestampPeriod = (fp64_t)mContext->PhysicalDevice.properties.limits.timestampPeriod;

        std::vector<const char*> test(queryNames);
        mQueryNames = queryNames;
        for(int32_t i = 0; i < mQueryNames.size(); i++)
        {
            mQueryIds[mQueryNames[i]] = i;
        }

        mQueryPools.resize(uniqueSets);

        for(uint32_t frameIndex = 0; frameIndex < uniqueSets; frameIndex++)
        {
            auto& pool = mQueryPools[frameIndex];

            VkQueryPoolCreateInfo poolCi{.sType              = VkStructureType::VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                         .pNext              = nullptr,
                                         .flags              = 0,
                                         .queryType          = VkQueryType::VK_QUERY_TYPE_TIMESTAMP,
                                         .queryCount         = static_cast<uint32_t>(mQueryNames.size()),
                                         .pipelineStatistics = 0};

            vkCreateQueryPool(mContext->Device, &poolCi, nullptr, &pool);
        }
    }

    void DeviceBenchmark::CmdResetQuery(VkCommandBuffer cmdBuffer, uint32_t frameIndex)
    {
        vkCmdResetQueryPool(cmdBuffer, mQueryPools[frameIndex % mQueryPools.size()], 0, mQueryNames.size());
    }
    void DeviceBenchmark::CmdWriteTimestamp(VkCommandBuffer cmdBuffer, uint32_t frameIndex, const char* name, VkPipelineStageFlagBits stageFlagBit)
    {
        vkCmdWriteTimestamp(cmdBuffer, stageFlagBit, mQueryPools[frameIndex % mQueryPools.size()], mQueryIds[name]);
    }

    struct QueryResult
    {
        uint64_t Timestamp;
        uint64_t Available;
    };

    bool DeviceBenchmark::LogQueryResults(uint32_t frameIndex)
    {
        std::vector<QueryResult> results(mQueryNames.size());
        vkGetQueryPoolResults(mContext->Device, mQueryPools[frameIndex % mQueryPools.size()], 0, mQueryNames.size(), sizeof(QueryResult) * results.size(), results.data(),
                              sizeof(QueryResult), VkQueryResultFlagBits::VK_QUERY_RESULT_64_BIT | VkQueryResultFlagBits::VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        if(results[0].Available == 0)
        {
            return false;
        }

        if(std::string_view(BenchmarkTimestamp::BEGIN) == mQueryNames.front() && std::string_view(BenchmarkTimestamp::END) == mQueryNames.back())
        {
            BenchmarkBase::Begin(ConvertQueryResultToMillis(results.front().Timestamp));
            for(int32_t id = 1; id < results.size() - 1; id++)
            {
                BenchmarkBase::LogTimestamp(mQueryNames[id], ConvertQueryResultToMillis(results[id].Timestamp));
            }
            BenchmarkBase::End(ConvertQueryResultToMillis(results.back().Timestamp));
        }
        else
        {
            BenchmarkBase::Begin(ConvertQueryResultToMillis(results.front().Timestamp));
            for(int32_t id = 0; id < results.size(); id++)
            {
                BenchmarkBase::LogTimestamp(mQueryNames[id], ConvertQueryResultToMillis(results[id].Timestamp));
            }
            BenchmarkBase::End(ConvertQueryResultToMillis(results.back().Timestamp));
        }

        return true;
    }

    fp64_t DeviceBenchmark::ConvertQueryResultToMillis(uint64_t result)
    {
        fp64_t nanos = result * mTimestampPeriod;
        return nanos / (1000.0 * 1000.0);
    }

    void DeviceBenchmark::Destroy()
    {
        for(auto pool : mQueryPools)
        {
            vkDestroyQueryPool(mContext->Device, pool, nullptr);
        }
        mQueryPools.clear();
    }
}  // namespace hsk