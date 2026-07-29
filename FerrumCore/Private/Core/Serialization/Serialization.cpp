#include <Core/Serialization/Serialization.h>
#include <Core/Strings/Format.h>

namespace FE::Serialization
{
    bool SerializationContext::Store(const Rtti::Type& type, const void* value)
    {
        if (type.m_serialize == nullptr || value == nullptr)
            return false;

        Begin();
        if (!m_format->BeginDocumentImpl(type.m_id, type.m_serializationVersion, type.m_serializationSchemaHash))
            return false;

        if (!type.m_serialize(*this, value))
            m_format->Fail(ErrorCode::kSerializerError);
        m_format->EndDocumentImpl();
        return IsValid();
    }


    bool DeserializationContext::Load(const Rtti::Type& type, void* value)
    {
        if (type.m_deserialize == nullptr || value == nullptr)
            return false;

        Begin();
        if (!m_format->BeginDocumentImpl(type.m_id, type.m_serializationVersion, type.m_serializationSchemaHash))
            return false;

        if (!type.m_deserialize(*this, value))
            m_format->Fail(ErrorCode::kSerializerError);
        m_format->EndDocumentImpl();
        return IsValid();
    }


    bool Serializer<Uuid>::Serialize(SerializationContext& context, const Uuid& value)
    {
        if (context.IsBinary())
        {
            context.RawBytes(value);
            return context.IsValid();
        }

        const festd::fixed_string formatted = Fmt::FixedFormat("{}", value);
        context.StoreString(formatted);
        return context.IsValid();
    }


    bool Serializer<Uuid>::Deserialize(DeserializationContext& context, Uuid& value)
    {
        if (context.IsBinary())
        {
            context.RawBytes(value);
            return context.IsValid();
        }

        const uint32_t size = context.LoadStringSize();
        if (!context.IsValid())
            return false;
        if (size != 36)
        {
            context.ReportError(ErrorCode::kInvalidString);
            return false;
        }

        char buffer[36];
        context.LoadString(festd::span<char>{ buffer });
        if (!context.IsValid())
            return false;

        value = Uuid::Parse(festd::ascii_view{ buffer, sizeof(buffer) });
        if (!value.IsValid())
            context.ReportError(ErrorCode::kInvalidString);
        return context.IsValid();
    }
} // namespace FE::Serialization
