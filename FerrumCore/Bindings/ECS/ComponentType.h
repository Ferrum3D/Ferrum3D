#pragma once
#include <FeCore/ECS/ComponentType.h>

namespace FE::ECS
{
    struct ComponentTypeBinding
    {
        GUID Type;
        UInt32 Alignment;
        UInt32 DataSize;

        [[nodiscard]] inline ComponentType Convert() const
        {
            ComponentType result;
            result.Type      = UUID::FromGUID(Type);
            result.Alignment = Alignment;
            result.DataSize  = DataSize;
            return result;
        }
    };
} // namespace FE::ECS
