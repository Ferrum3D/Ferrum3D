#include <Core/Serialization/Serialization.h>
#include <Core/Strings/Format.h>

namespace FE::Serialization
{
    bool SerializationContext::Store(const Rtti::Type& type, const void* value)
    {
        if (type.m_serialize == nullptr || value == nullptr)
            return false;

        Begin(Direction::kSerialize);
        if (!BeginDocument(type.m_id, type.m_serializationVersion, type.m_serializationSchemaHash))
            return false;

        if (!type.m_serialize(*this, value))
            Fail(ErrorCode::kSerializerError);
        EndDocument();
        return m_isValid;
    }


    bool SerializationContext::Load(const Rtti::Type& type, void* value)
    {
        if (type.m_deserialize == nullptr || value == nullptr)
            return false;

        Begin(Direction::kDeserialize);
        if (!BeginDocument(type.m_id, type.m_serializationVersion, type.m_serializationSchemaHash))
            return false;

        if (!type.m_deserialize(*this, value))
            Fail(ErrorCode::kSerializerError);
        EndDocument();
        return m_isValid;
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


    bool Serializer<Uuid>::Deserialize(SerializationContext& context, Uuid& value)
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
