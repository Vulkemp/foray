#include "foray_devicebenchmark.hpp"
#include "../core/foray_context.hpp"

namespace foray::bench {
    void DeviceBenchmark::Create(core::Context* context, const std::vector<const char*>& queryNames, uint32_t uniqueSets)
    {
        mContext = context;

        mTimestampPeriod = (fp64_t)mContext->VkbPhysicalDevice->properties.limits.timestampPeriod;

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

            mContext->VkbDispatchTable->createQueryPool(&poolCi, nullptr, &pool);
        }
    }

    void DeviceBenchmark::CmdResetQuery(VkCommandBuffer cmdBuffer, uint64_t frameIndex)
    {
        mContext->VkbDispatchTable->cmdResetQueryPool(cmdBuffer, mQueryPools[frameIndex % mQueryPools.size()], 0, mQueryNames.size());
    }
    void DeviceBenchmark::CmdWriteTimestamp(VkCommandBuffer cmdBuffer, uint64_t frameIndex, const char* name, VkPipelineStageFlagBits stageFlagBit)
    {
        mContext->VkbDispatchTable->cmdWriteTimestamp(cmdBuffer, stageFlagBit, mQueryPools[frameIndex % mQueryPools.size()], mQueryIds[name]);
    }

    struct QueryResult
    {
        uint64_t Timestamp;
        uint64_t Available;
    };

    bool DeviceBenchmark::LogQueryResults(uint64_t frameIndex)
    {
        std::vector<QueryResult> results(mQueryNames.size());
        mContext->VkbDispatchTable->getQueryPoolResults(mQueryPools[frameIndex % mQueryPools.size()], 0, mQueryNames.size(), sizeof(QueryResult) * results.size(), results.data(),
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
            mContext->VkbDispatchTable->destroyQueryPool(pool, nullptr);
        }
        mQueryPools.clear();
    }
}  // namespace foray