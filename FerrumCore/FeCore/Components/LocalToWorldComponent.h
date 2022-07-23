#pragma once
#include <FeCore/Math/Matrix4x4F.h>

namespace FE::ECS
{
    struct alignas(16) LocalToWorldComponent
    {
        Matrix4x4F Matrix;

        FE_STRUCT_RTTI(LocalToWorldComponent, "79E8B950-E483-45F7-B08B-07DABC47D3DA");

        inline LocalToWorldComponent() = default;

        inline explicit LocalToWorldComponent(const Matrix4x4F& matrix)
            : Matrix(matrix)
        {
        }

        friend bool operator==(const LocalToWorldComponent& lhs, const LocalToWorldComponent& rhs)
        {
            return lhs.Matrix == rhs.Matrix;
        }

        friend bool operator!=(const LocalToWorldComponent& lhs, const LocalToWorldComponent& rhs)
        {
            return !(rhs == lhs);
        }
    };
} // namespace FE::ECS
