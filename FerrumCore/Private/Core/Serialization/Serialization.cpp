#include <Core/Serialization/Serialization.h>

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
            Fail();
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
            Fail();
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

        static constexpr char kHex[] = "0123456789abcdef";
        char buffer[36];
        uint32_t outputIndex = 0;
        for (uint32_t byteIndex = 0; byteIndex < 16; ++byteIndex)
        {
            if (byteIndex == 4 || byteIndex == 6 || byteIndex == 8 || byteIndex == 10)
                buffer[outputIndex++] = '-';

            buffer[outputIndex++] = kHex[value.m_bytes[byteIndex] >> 4];
            buffer[outputIndex++] = kHex[value.m_bytes[byteIndex] & 0xf];
        }

        context.String(festd::string_view{ buffer, sizeof(buffer) });
        return context.IsValid();
    }


    bool Serializer<Uuid>::Deserialize(SerializationContext& context, Uuid& value)
    {
        if (context.IsBinary())
        {
            context.RawBytes(value);
            return context.IsValid();
        }

        festd::string string;
        context.String(string);
        if (!context.IsValid())
            return false;

        value = Uuid::Parse(festd::ascii_view{ string.data(), string.size() });
        return value.IsValid();
    }
} // namespace FE::Serialization
