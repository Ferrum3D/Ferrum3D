#pragma once
#include <FeCore/Base/Base.h>

namespace FE::ECS
{
    namespace EntityConstants
    {
        //! \brief Number of bits that store entity archetype index.
        inline constexpr UInt32 ArchetypeBitCount = 16;

        //! \brief Number of bits that store entity ID within an archetype.
        inline constexpr UInt32 EntityIDBitCount = 32 - ArchetypeBitCount;
    } // namespace EntityConstants

    using EntityID = UInt32;

    enum class ECSResult
    {
        Success,
        AllocationError,
        EntityExists,
        EntityNotFound
    };
} // namespace FE::ECS
