#include <Core/Serialization/Serialization.h>
#include <Core/Strings/Format.h>

namespace FE::Serialization
{
    ResultCode SerializationFormat::Fail(const ResultCode code, const uint64_t byteOffset, const uint32_t line,
                                         const uint32_t column)
    {
        if (code == ResultCode::kSuccess)
            return m_error.m_code;
        if (m_error.m_code != ResultCode::kSuccess)
            return m_error.m_code;

        m_error.m_code = code;
        m_error.m_byteOffset = byteOffset == UINT64_MAX ? GetCurrentOffsetImpl() : byteOffset;
        m_error.m_line = line;
        m_error.m_column = column;
        return code;
    }


    ResultCode SerializationFormat::BeginDocument(IO::IStream* stream, const Direction direction, const Rtti::TypeID expectedType,
                                                  const uint32_t version, const uint64_t schemaHash)
    {
        FE_Assert(!m_isDocumentActive);
        m_stream = stream;
        m_direction = direction;
        m_error = {};
        m_serializedVersion = 0;
        m_serializedSchemaHash = 0;
        ResetImpl();
        m_isDocumentActive = true;

        const ResultCode result = BeginDocumentImpl(expectedType, version, schemaHash);
        Fail(result);
        if (result != ResultCode::kSuccess)
            EndDocument();

        return result;
    }


    ResultCode SerializationFormat::EndDocument()
    {
        if (!m_isDocumentActive)
            return m_error.m_code;

        Fail(EndDocumentImpl());
        const ResultCode result = m_error.m_code;
        ResetImpl();
        m_stream = nullptr;
        m_isDocumentActive = false;
        return result;
    }


    SerializationObject SerializationContext::BeginObject()
    {
        if (!IsValid())
            return SerializationObject{ nullptr };
        return SerializationObject{ m_format->Record(m_format->BeginObjectImpl()) == ResultCode::kSuccess ? this : nullptr };
    }


    SerializationArray SerializationContext::BeginArray(const uint32_t size)
    {
        if (!IsValid())
            return SerializationArray{ nullptr };

        uint32_t mutableSize = size;
        return SerializationArray{ m_format->Record(m_format->BeginArrayImpl(mutableSize)) == ResultCode::kSuccess ? this
                                                                                                                   : nullptr };
    }


    DeserializationObject DeserializationContext::BeginObject()
    {
        if (!IsValid())
            return DeserializationObject{ nullptr };

        return DeserializationObject{ m_format->Record(m_format->BeginObjectImpl()) == ResultCode::kSuccess ? this : nullptr };
    }


    DeserializationArray DeserializationContext::BeginArray(uint32_t& size)
    {
        if (!IsValid())
            return DeserializationArray{ nullptr };

        return DeserializationArray{ m_format->Record(m_format->BeginArrayImpl(size)) == ResultCode::kSuccess ? this : nullptr };
    }


    ResultCode SerializationContext::Store(const Rtti::Type& type, const void* value)
    {
        if (type.m_serialize == nullptr || value == nullptr)
            return ResultCode::kSerializerError;

        const ResultCode result = m_format->BeginDocument(m_stream,
                                                          Direction::kSerialize,
                                                          type.m_id,
                                                          type.m_serializationVersion,
                                                          type.m_serializationSchemaHash);
        if (result != ResultCode::kSuccess)
            return result;

        m_format->Record(type.m_serialize(*this, value));
        return m_format->EndDocument();
    }


    ResultCode DeserializationContext::Load(const Rtti::Type& type, void* value)
    {
        if (type.m_deserialize == nullptr || value == nullptr)
            return ResultCode::kSerializerError;

        const ResultCode result = m_format->BeginDocument(m_stream,
                                                          Direction::kDeserialize,
                                                          type.m_id,
                                                          type.m_serializationVersion,
                                                          type.m_serializationSchemaHash);
        if (result != ResultCode::kSuccess)
            return result;

        m_format->Record(type.m_deserialize(*this, value));
        return m_format->EndDocument();
    }


    ResultCode Serializer<Uuid>::Serialize(SerializationContext& context, const Uuid& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        const festd::fixed_string formatted = Fmt::FixedFormat("{}", value);
        return context.StoreString(formatted);
    }


    ResultCode Serializer<Uuid>::Deserialize(DeserializationContext& context, Uuid& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        uint32_t size = 0;
        ResultCode result = context.LoadStringSize(size);
        if (result != ResultCode::kSuccess)
            return result;

        if (size != 36)
            return context.ReportError(ResultCode::kInvalidString);

        char buffer[36];
        result = context.LoadString(festd::span<char>{ buffer });
        if (result != ResultCode::kSuccess)
            return result;

        value = Uuid::Parse(festd::ascii_view{ buffer, sizeof(buffer) });
        if (!value.IsValid())
            return context.ReportError(ResultCode::kInvalidString);

        return ResultCode::kSuccess;
    }
} // namespace FE::Serialization
