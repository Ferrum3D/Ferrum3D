#pragma once
#include <FeCore/Base/Base.h>

namespace FE::ECS
{
    namespace EntityConstants
    {
        inline constexpr UInt32 VersionBitCount  = 10;
        inline constexpr UInt32 EntityIDBitCount = 32 - VersionBitCount;
        inline constexpr UInt32 EntityIDMask = MakeMask(EntityIDBitCount, 0u);
        inline constexpr UInt32 VersionMask = MakeMask(VersionBitCount, EntityIDBitCount);
    } // namespace EntityConstants

    using EntityID = UInt32;

    enum class ECSResult
    {
        Success,
        OutOfMemoryError,
        AllocationError,
        OutOfRangeError,
        ComponentNotFoundError
    };
} // namespace FE::ECS
