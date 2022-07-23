#pragma once
#include <FeCore/Math/VectorMath.h>

namespace FE::ECS
{
    struct alignas(4) Position2DComponent
    {
        float X = 0.0f;
        float Y = 0.0f;

        FE_STRUCT_RTTI(Position2DComponent, "930DFC8B-1340-4C39-A0A9-CD44F0E2E5D6");

        inline Position2DComponent() = default;

        inline Position2DComponent(float x, float y)
            : X(x)
            , Y(y)
        {
        }

        inline explicit Position2DComponent(const Vector2F& vector)
        {
            memcpy(this, vector.Data(), sizeof(*this));
        }

        inline explicit Position2DComponent(const Vector3F& vector)
        {
            memcpy(this, vector.Data(), sizeof(*this));
        }

        inline explicit Position2DComponent(const Vector4F& vector)
        {
            memcpy(this, vector.Data(), sizeof(*this));
        }

        [[nodiscard]] inline Vector2F AsVector2F() const
        {
            return { X, Y };
        }

        [[nodiscard]] inline Vector3F AsVector3F() const
        {
            return { X, Y, 0 };
        }

        [[nodiscard]] inline Vector4F AsVector4F() const
        {
            return { X, Y, 0, 0 };
        }

        friend bool operator==(const Position2DComponent& lhs, const Position2DComponent& rhs)
        {
            return lhs.X == rhs.X && lhs.Y == rhs.Y;
        }

        friend bool operator!=(const Position2DComponent& lhs, const Position2DComponent& rhs)
        {
            return !(rhs == lhs);
        }
    };

    struct alignas(4) Position3DComponent
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;

        FE_STRUCT_RTTI(Position3DComponent, "1F3CD11C-5547-4773-9941-082C257C6729");

        inline Position3DComponent() = default;

        inline Position3DComponent(float x, float y, float z)
            : X(x)
            , Y(y)
            , Z(z)
        {
        }

        inline explicit Position3DComponent(const Vector2F& vector)
        {
            memcpy(this, vector.Data(), sizeof(float) * 2);
        }

        inline explicit Position3DComponent(const Vector3F& vector)
        {
            memcpy(this, vector.Data(), sizeof(*this));
        }

        inline explicit Position3DComponent(const Vector4F& vector)
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

        friend bool operator==(const Position3DComponent& lhs, const Position3DComponent& rhs)
        {
            return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
        }

        friend bool operator!=(const Position3DComponent& lhs, const Position3DComponent& rhs)
        {
            return !(rhs == lhs);
        }
    };
} // namespace FE::ECS
