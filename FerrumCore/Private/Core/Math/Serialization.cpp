#include <Core/Math/Aabb.h>
#include <Core/Math/Color.h>
#include <Core/Math/Matrix4x4.h>
#include <Core/Math/Quaternion.h>
#include <Core/Math/Rect.h>
#include <Core/Math/Sphere.h>
#include <Core/Math/Vector2.h>
#include <Core/Math/Vector3.h>
#include <Core/Math/Vector3Int.h>
#include <Core/Math/Vector3UInt.h>
#include <Core/Math/Vector4.h>
#include <Core/Serialization/Serialization.h>

namespace FE
{
    namespace
    {
        template<class T>
        struct NamedField final
        {
            festd::ascii_view m_name;
            T& m_value;
        };


        template<class T>
        NamedField<T> MakeField(const festd::ascii_view name, T& value)
        {
            return { name, value };
        }


        template<class TContext, class... TFields>
        Serialization::ResultCode TransferObject(TContext& context, TFields... fields)
        {
            if (auto object = context.BeginObject())
            {
                (object.Field(fields.m_name, fields.m_value), ...);
            }
            else
            {
                return context.GetResultCode();
            }

            return context.GetResultCode();
        }

    } // namespace


    template<class T>
    Serialization::ResultCode Vector2Base<T>::Serialize(Serialization::SerializationContext& context, const Vector2Base<T>& value)
    {
        return TransferObject(context, MakeField("x", value.x), MakeField("y", value.y));
    }


    template<class T>
    Serialization::ResultCode Vector2Base<T>::Deserialize(Serialization::DeserializationContext& context, Vector2Base<T>& value)
    {
        return TransferObject(context, MakeField("x", value.x), MakeField("y", value.y));
    }


    template<class T>
    Serialization::ResultCode RectBase<T>::Serialize(Serialization::SerializationContext& context, const RectBase<T>& value)
    {
        return TransferObject(context, MakeField("min", value.min), MakeField("max", value.max));
    }


    template<class T>
    Serialization::ResultCode RectBase<T>::Deserialize(Serialization::DeserializationContext& context, RectBase<T>& value)
    {
        return TransferObject(context, MakeField("min", value.min), MakeField("max", value.max));
    }


    Serialization::ResultCode Vector3::Serialize(Serialization::SerializationContext& context, const Vector3& value)
    {
        return TransferObject(context, MakeField("x", value.x), MakeField("y", value.y), MakeField("z", value.z));
    }


    Serialization::ResultCode Vector3::Deserialize(Serialization::DeserializationContext& context, Vector3& value)
    {
        float newX = value.x;
        float newY = value.y;
        float newZ = value.z;
        const Serialization::ResultCode result =
            TransferObject(context, MakeField("x", newX), MakeField("y", newY), MakeField("z", newZ));
        if (result == Serialization::ResultCode::kSuccess)
            value.m_simdVector = _mm_setr_ps(newX, newY, newZ, 0.0f);
        return result;
    }


    Serialization::ResultCode Vector3Int::Serialize(Serialization::SerializationContext& context, const Vector3Int& value)
    {
        return TransferObject(context, MakeField("x", value.x), MakeField("y", value.y), MakeField("z", value.z));
    }


    Serialization::ResultCode Vector3Int::Deserialize(Serialization::DeserializationContext& context, Vector3Int& value)
    {
        int32_t newX = value.x;
        int32_t newY = value.y;
        int32_t newZ = value.z;
        const Serialization::ResultCode result =
            TransferObject(context, MakeField("x", newX), MakeField("y", newY), MakeField("z", newZ));
        if (result == Serialization::ResultCode::kSuccess)
            value.m_simdVector = _mm_setr_epi32(newX, newY, newZ, 0);
        return result;
    }


    Serialization::ResultCode Vector3UInt::Serialize(Serialization::SerializationContext& context, const Vector3UInt& value)
    {
        return TransferObject(context, MakeField("x", value.x), MakeField("y", value.y), MakeField("z", value.z));
    }


    Serialization::ResultCode Vector3UInt::Deserialize(Serialization::DeserializationContext& context, Vector3UInt& value)
    {
        uint32_t newX = value.x;
        uint32_t newY = value.y;
        uint32_t newZ = value.z;
        const Serialization::ResultCode result =
            TransferObject(context, MakeField("x", newX), MakeField("y", newY), MakeField("z", newZ));
        if (result == Serialization::ResultCode::kSuccess)
        {
            value.m_simdVector =
                _mm_setr_epi32(static_cast<int32_t>(newX), static_cast<int32_t>(newY), static_cast<int32_t>(newZ), 0);
        }
        return result;
    }


#define FE_DEFINE_VECTOR3_SERIALIZATION(type)                                                                                    \
    Serialization::ResultCode type::Serialize(Serialization::SerializationContext& context, const type& value)                   \
    {                                                                                                                            \
        return TransferObject(context, MakeField("x", value.x), MakeField("y", value.y), MakeField("z", value.z));               \
    }                                                                                                                            \
                                                                                                                                 \
    Serialization::ResultCode type::Deserialize(Serialization::DeserializationContext& context, type& value)                     \
    {                                                                                                                            \
        return TransferObject(context, MakeField("x", value.x), MakeField("y", value.y), MakeField("z", value.z));               \
    }

    FE_DEFINE_VECTOR3_SERIALIZATION(PackedVector3F)
    FE_DEFINE_VECTOR3_SERIALIZATION(PackedVector3Int)
    FE_DEFINE_VECTOR3_SERIALIZATION(PackedVector3UInt)

#undef FE_DEFINE_VECTOR3_SERIALIZATION


#define FE_DEFINE_VECTOR4_SERIALIZATION(type, field0, field1, field2, field3)                                                    \
    Serialization::ResultCode type::Serialize(Serialization::SerializationContext& context, const type& value)                   \
    {                                                                                                                            \
        return TransferObject(context,                                                                                           \
                              MakeField(#field0, value.field0),                                                                  \
                              MakeField(#field1, value.field1),                                                                  \
                              MakeField(#field2, value.field2),                                                                  \
                              MakeField(#field3, value.field3));                                                                 \
    }                                                                                                                            \
                                                                                                                                 \
    Serialization::ResultCode type::Deserialize(Serialization::DeserializationContext& context, type& value)                     \
    {                                                                                                                            \
        return TransferObject(context,                                                                                           \
                              MakeField(#field0, value.field0),                                                                  \
                              MakeField(#field1, value.field1),                                                                  \
                              MakeField(#field2, value.field2),                                                                  \
                              MakeField(#field3, value.field3));                                                                 \
    }

    FE_DEFINE_VECTOR4_SERIALIZATION(Vector4, x, y, z, w)
    FE_DEFINE_VECTOR4_SERIALIZATION(PackedVector4F, x, y, z, w)
    FE_DEFINE_VECTOR4_SERIALIZATION(Quaternion, x, y, z, w)
    FE_DEFINE_VECTOR4_SERIALIZATION(Color4F, r, g, b, a)

#undef FE_DEFINE_VECTOR4_SERIALIZATION


    Serialization::ResultCode Matrix4x4::Serialize(Serialization::SerializationContext& context, const Matrix4x4& value)
    {
        return TransferObject(context, MakeField("m_rows", value.m_rows));
    }


    Serialization::ResultCode Matrix4x4::Deserialize(Serialization::DeserializationContext& context, Matrix4x4& value)
    {
        return TransferObject(context, MakeField("m_rows", value.m_rows));
    }


    Serialization::ResultCode Aabb::Serialize(Serialization::SerializationContext& context, const Aabb& value)
    {
        return TransferObject(context, MakeField("min", value.min), MakeField("max", value.max));
    }


    Serialization::ResultCode Aabb::Deserialize(Serialization::DeserializationContext& context, Aabb& value)
    {
        return TransferObject(context, MakeField("min", value.min), MakeField("max", value.max));
    }


    Serialization::ResultCode PackedAabb::Serialize(Serialization::SerializationContext& context, const PackedAabb& value)
    {
        return TransferObject(context, MakeField("min", value.min), MakeField("max", value.max));
    }


    Serialization::ResultCode PackedAabb::Deserialize(Serialization::DeserializationContext& context, PackedAabb& value)
    {
        return TransferObject(context, MakeField("min", value.min), MakeField("max", value.max));
    }


    Serialization::ResultCode Sphere::Serialize(Serialization::SerializationContext& context, const Sphere& value)
    {
        const Vector3 center = value.Center();
        const float radius = value.Radius();
        return TransferObject(context, MakeField("center", center), MakeField("radius", radius));
    }


    Serialization::ResultCode Sphere::Deserialize(Serialization::DeserializationContext& context, Sphere& value)
    {
        Vector3 center = value.Center();
        float radius = value.Radius();
        const Serialization::ResultCode result =
            TransferObject(context, MakeField("center", center), MakeField("radius", radius));
        if (result == Serialization::ResultCode::kSuccess)
            value.m_centerRadius = Vector4{ center, radius };
        return result;
    }


#define FE_INSTANTIATE_MATH_SERIALIZATION(type)                                                                                  \
    template Serialization::ResultCode type::Serialize(Serialization::SerializationContext&, const type&);                       \
    template Serialization::ResultCode type::Deserialize(Serialization::DeserializationContext&, type&)

    FE_INSTANTIATE_MATH_SERIALIZATION(Vector2Base<float>);
    FE_INSTANTIATE_MATH_SERIALIZATION(Vector2Base<int32_t>);
    FE_INSTANTIATE_MATH_SERIALIZATION(Vector2Base<uint32_t>);
    FE_INSTANTIATE_MATH_SERIALIZATION(RectBase<float>);
    FE_INSTANTIATE_MATH_SERIALIZATION(RectBase<int32_t>);
    FE_INSTANTIATE_MATH_SERIALIZATION(RectBase<uint32_t>);

#undef FE_INSTANTIATE_MATH_SERIALIZATION
} // namespace FE
