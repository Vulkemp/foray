#include "managedresource.hpp"
#include "../logger.hpp"
#include <nameof/nameof.hpp>
#include <spdlog/fmt/fmt.h>
#include <sstream>

namespace foray::core {
    void ManagedResource::sPrintAllocatedResources(bool printAsWarning)
    {
        uint32_t allocatedResourceCount = 0;
        for(ManagedResource* resource : sAllocatedRessources)
        {
            if(resource->Exists())
            {
                allocatedResourceCount++;
            }
        }

        if(allocatedResourceCount == 0)
        {
            logger()->info("[ManagedResource] No resources allocated.");
            return;
        }
        std::stringstream strbuilder;
        if(allocatedResourceCount == 1)
        {
            strbuilder << "1 allocated resource:";
        }
        else
        {
            strbuilder << allocatedResourceCount << " allocated resources:";
        }
        for(ManagedResource* resource : sAllocatedRessources)
        {
            if(resource->Exists())
            {
                strbuilder << "\n   " << resource->Print();
            }
        }
        spdlog::level::level_enum level = printAsWarning ? spdlog::level::level_enum::warn : spdlog::level::level_enum::info;
        logger()->log(level, strbuilder.str());
    }

    std::string_view ManagedResource::GetTypeName() const
    {
        return "";
    }

    ManagedResource::ManagedResource()
    {
        sAllocatedRessources.insert(this);
    }

    ManagedResource::ManagedResource(std::string_view name) : mName(name)
    {
        sAllocatedRessources.insert(this);
    }

    ManagedResource::~ManagedResource()
    {
        sAllocatedRessources.erase(this);
    }

    void ManagedResource::SetName(std::string_view name)
    {
        mName = name;
    }

    std::string ManagedResource::Print() const
    {
        std::string_view name = GetName();
        std::string_view type = GetTypeName();

        if(name.size() == 0)
        {
            name = "Unnamed Object";
        }
        if(type.size() == 0)
        {
            type = "Unknown Type";
        }

        return fmt::format("\"{}\" ({})", name, type);
    }

    std::string_view PrintVkObjectType(vk::ObjectType objectType)
    {
        if(objectType > NAMEOF_ENUM_RANGE_MAX)
        {
            return "vk::ObjectType (Out of NAMEOF_ENUM_RANGE)";
        }
        else
        {
            return NAMEOF_ENUM(objectType);
        }
    }
}  // namespace foray::core
