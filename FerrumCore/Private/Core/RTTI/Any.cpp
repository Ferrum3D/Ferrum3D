#include <Core/RTTI/Any.h>

#include <Core/Serialization/Serialization.h>

namespace FE::Rtti
{
    Any::~Any()
    {
        Reset();
    }


    Any::Any(const Any& other)
    {
        if (!other.HasValue())
            return;

        FE_Assert(other.m_type->m_copyConstructor != nullptr, "Any value is not copy constructible");
        m_type = other.m_type;
        m_value = Memory::DefaultAllocate(m_type->m_size, m_type->m_alignment);
        m_type->m_copyConstructor(m_value, other.m_value);
    }


    Any& Any::operator=(const Any& other)
    {
        if (this == &other)
            return *this;

        Reset();
        if (!other.HasValue())
            return *this;

        FE_Assert(other.m_type->m_copyConstructor != nullptr, "Any value is not copy constructible");
        m_type = other.m_type;
        m_value = Memory::DefaultAllocate(m_type->m_size, m_type->m_alignment);
        m_type->m_copyConstructor(m_value, other.m_value);
        return *this;
    }


    Any::Any(Any&& other) noexcept
        : m_type(other.m_type)
        , m_value(other.m_value)
    {
        other.m_type = nullptr;
        other.m_value = nullptr;
    }


    Any& Any::operator=(Any&& other) noexcept
    {
        if (this == &other)
            return *this;

        Reset();
        m_type = other.m_type;
        m_value = other.m_value;
        other.m_type = nullptr;
        other.m_value = nullptr;
        return *this;
    }


    void Any::Reset()
    {
        if (m_value == nullptr)
            return;

        FE_Assert(m_type != nullptr && m_type->m_destructor != nullptr);
        m_type->m_destructor(m_value);
        Memory::DefaultFree(m_value);
        m_type = nullptr;
        m_value = nullptr;
    }


    Serialization::ResultCode Any::Serialize(Serialization::SerializationContext& context) const
    {
        if (auto object = context.BeginObject())
        {
            const auto typeId = m_type != nullptr ? m_type->m_id : TypeID::kNull;
            object.Field("m_typeId", typeId);
            if (m_type != nullptr)
                object.Field("m_value", *m_type, m_value);
        }

        return context.GetResultCode();
    }


    Serialization::ResultCode Any::Deserialize(Serialization::DeserializationContext& context)
    {
        Reset();

        auto typeId = TypeID::kNull;
        if (auto object = context.BeginObject())
        {
            object.Field("m_typeId", typeId);
            if (context.IsValid() && typeId.IsValid())
            {
                const Type* type = TypeRegistry::FindType(typeId);
                if (type == nullptr || type->m_defaultConstructor == nullptr || type->m_deserialize == nullptr)
                {
                    context.ReportError(Serialization::ResultCode::kTypeMismatch);
                }
                else
                {
                    m_type = type;
                    m_value = Memory::DefaultAllocate(type->m_size, type->m_alignment);
                    type->m_defaultConstructor(m_value);
                    object.Field("m_value", *type, m_value);
                }
            }
        }

        if (!context.IsValid())
            Reset();

        return context.GetResultCode();
    }


    uint64_t Any::RTTI_GetSerializationSchemaHash()
    {
        return TypeNameHash<Any>;
    }


    uint32_t Any::RTTI_GetSerializationVersion()
    {
        return 0;
    }
} // namespace FE::Rtti
