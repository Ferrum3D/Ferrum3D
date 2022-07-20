#pragma once
#include <FeCore/Math/VectorMath.h>

namespace FE::ECS
{
    struct alignas(8) PositionComponent
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;

        FE_STRUCT_RTTI(PositionComponent, "1F3CD11C-5547-4773-9941-082C257C6729");

        inline PositionComponent() = default;

        inline PositionComponent(float x, float y, float z = 0.0f)
            : X(x)
            , Y(y)
            , Z(z)
        {
        }

        inline explicit PositionComponent(const Vector2F& vector)
        {
            memcpy(this, vector.Data(), sizeof(float) * 2);
        }

        inline explicit PositionComponent(const Vector3F& vector)
        {
            memcpy(this, vector.Data(), sizeof(*this));
        }

        inline explicit PositionComponent(const Vector4F& vector)
        {
            memcpy(this, vector.Data(), sizeof(*this));
        }

        [[nodiscard]] inline Vector2F AsVector2F() const
        {
            return { X, Y };
        }

        [[nodiscard]] inline Vector3F AsVector3F() const
        {
            return { X, Y, Z };
        }

        [[nodiscard]] inline Vector4F AsVector4F() const
        {
            return { X, Y, Z, 0 };
        }
    };
} // namespace FE::ECS
