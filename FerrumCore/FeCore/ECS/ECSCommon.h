#pragma once
#include <FeCore/Base/Base.h>

namespace FE::ECS
{
    namespace EntityConstants
    {
        inline constexpr uint32_t VersionBitCount  = 10;
        inline constexpr uint32_t EntityIDBitCount = 32 - VersionBitCount;
        inline constexpr uint32_t EntityIDMask = MakeMask(EntityIDBitCount, 0u);
        inline constexpr uint32_t VersionMask = MakeMask(VersionBitCount, EntityIDBitCount);
    } // namespace EntityConstants

    using EntityID = uint32_t;

    enum class ECSResult
    {
        Success,
        OutOfMemoryError,
        AllocationError,
        OutOfRangeError,
        ComponentNotFoundError
    };
} // namespace FE::ECS
