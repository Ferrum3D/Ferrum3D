#include <Core/Serialization/JsonSerialization.h>
#include <charconv>
#include <cmath>
#include <cstdlib>
FE_PUSH_MSVC_WARNING(5054)
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
FE_POP_MSVC_WARNING()

namespace FE::Serialization
{
    namespace
    {
        void FormatUuid(const Rtti::TypeID value, char (&buffer)[36])
        {
            static constexpr char kHex[] = "0123456789abcdef";
            uint32_t outputIndex = 0;
            for (uint32_t byteIndex = 0; byteIndex < 16; ++byteIndex)
            {
                if (byteIndex == 4 || byteIndex == 6 || byteIndex == 8 || byteIndex == 10)
                    buffer[outputIndex++] = '-';

                buffer[outputIndex++] = kHex[value.m_bytes[byteIndex] >> 4];
                buffer[outputIndex++] = kHex[value.m_bytes[byteIndex] & 0xf];
            }
        }


        uint64_t LoadUnsigned(const void* value, const uint32_t byteSize)
        {
            switch (byteSize)
            {
            case 1:
                return *static_cast<const uint8_t*>(value);
            case 2:
                return *static_cast<const uint16_t*>(value);
            case 4:
                return *static_cast<const uint32_t*>(value);
            case 8:
                return *static_cast<const uint64_t*>(value);
            default:
                return 0;
            }
        }


        int64_t LoadSigned(const void* value, const uint32_t byteSize)
        {
            switch (byteSize)
            {
            case 1:
                return *static_cast<const int8_t*>(value);
            case 2:
                return *static_cast<const int16_t*>(value);
            case 4:
                return *static_cast<const int32_t*>(value);
            case 8:
                return *static_cast<const int64_t*>(value);
            default:
                return 0;
            }
        }


        bool StoreUnsigned(void* destination, const uint32_t byteSize, const uint64_t value)
        {
            switch (byteSize)
            {
            case 1:
                if (value > UINT8_MAX)
                    return false;
                *static_cast<uint8_t*>(destination) = static_cast<uint8_t>(value);
                return true;
            case 2:
                if (value > UINT16_MAX)
                    return false;
                *static_cast<uint16_t*>(destination) = static_cast<uint16_t>(value);
                return true;
            case 4:
                if (value > UINT32_MAX)
                    return false;
                *static_cast<uint32_t*>(destination) = static_cast<uint32_t>(value);
                return true;
            case 8:
                *static_cast<uint64_t*>(destination) = value;
                return true;
            default:
                return false;
            }
        }


        bool StoreSigned(void* destination, const uint32_t byteSize, const int64_t value)
        {
            switch (byteSize)
            {
            case 1:
                if (value < INT8_MIN || value > INT8_MAX)
                    return false;
                *static_cast<int8_t*>(destination) = static_cast<int8_t>(value);
                return true;
            case 2:
                if (value < INT16_MIN || value > INT16_MAX)
                    return false;
                *static_cast<int16_t*>(destination) = static_cast<int16_t>(value);
                return true;
            case 4:
                if (value < INT32_MIN || value > INT32_MAX)
                    return false;
                *static_cast<int32_t*>(destination) = static_cast<int32_t>(value);
                return true;
            case 8:
                *static_cast<int64_t*>(destination) = value;
                return true;
            default:
                return false;
            }
        }
    } // namespace


    struct JsonContext::Impl final
    {
        using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;

        explicit Impl(IO::IStream* stream)
            : m_stream(stream)
        {
        }

        IO::IStream* m_stream;
        rapidjson::StringBuffer m_output;
        std::unique_ptr<Writer> m_writer;
        rapidjson::Document m_document;
        festd::vector<rapidjson::Value*> m_inputValues;
        festd::vector<char> m_input;
    };


    JsonContext::JsonContext(IO::IStream* stream)
        : SerializationContext(Format::kJson)
        , m_impl(std::make_unique<Impl>(stream))
    {
        FE_Assert(stream != nullptr);
    }


    JsonContext::~JsonContext() = default;


    void JsonContext::Reset()
    {
        m_impl->m_output.Clear();
        m_impl->m_writer.reset();
        m_impl->m_document.SetNull();
        m_impl->m_inputValues.clear();
        m_impl->m_input.clear();
    }


    bool JsonContext::BeginDocument(const Rtti::TypeID expectedType, const uint32_t version, const uint64_t schemaHash)
    {
        if (IsSerializing())
        {
            m_impl->m_writer = std::make_unique<Impl::Writer>(m_impl->m_output);
            auto& writer = *m_impl->m_writer;
            writer.StartObject();

            char typeString[36];
            FormatUuid(expectedType, typeString);
            writer.Key("$type");
            writer.String(typeString, sizeof(typeString));
            writer.Key("$version");
            writer.Uint(version);

            char schemaString[19];
            schemaString[0] = '0';
            schemaString[1] = 'x';
            const auto schemaResult = std::to_chars(schemaString + 2, schemaString + sizeof(schemaString), schemaHash, 16);
            writer.Key("$schema");
            writer.String(schemaString, static_cast<rapidjson::SizeType>(schemaResult.ptr - schemaString));
            writer.Key("$value");
            return true;
        }

        char chunk[4096];
        while (true)
        {
            const size_t bytesRead = m_impl->m_stream->ReadToBuffer(chunk, sizeof(chunk));
            m_impl->m_input.insert(m_impl->m_input.end(), chunk, chunk + bytesRead);
            if (bytesRead != sizeof(chunk))
                break;
        }
        m_impl->m_input.push_back('\0');

        m_impl->m_document.Parse(m_impl->m_input.data());
        if (m_impl->m_document.HasParseError() || !m_impl->m_document.IsObject() || !m_impl->m_document.HasMember("$type")
            || !m_impl->m_document.HasMember("$version") || !m_impl->m_document.HasMember("$schema")
            || !m_impl->m_document.HasMember("$value"))
        {
            Fail();
            return false;
        }

        const auto& typeValue = m_impl->m_document["$type"];
        const auto& versionValue = m_impl->m_document["$version"];
        const auto& schemaValue = m_impl->m_document["$schema"];
        if (!typeValue.IsString() || !versionValue.IsUint() || !schemaValue.IsString())
        {
            Fail();
            return false;
        }

        const Rtti::TypeID serializedType =
            Rtti::TypeID::Parse(festd::ascii_view{ typeValue.GetString(), typeValue.GetStringLength() });
        if (expectedType.IsValid() && serializedType != expectedType)
        {
            Fail();
            return false;
        }

        m_serializedVersion = versionValue.GetUint();
        const char* schemaBegin = schemaValue.GetString();
        const char* schemaEnd = schemaBegin + schemaValue.GetStringLength();
        if (schemaEnd - schemaBegin >= 2 && schemaBegin[0] == '0' && schemaBegin[1] == 'x')
            schemaBegin += 2;
        const auto schemaResult = std::from_chars(schemaBegin, schemaEnd, m_serializedSchemaHash, 16);
        if (schemaResult.ec != std::errc{} || schemaResult.ptr != schemaEnd)
        {
            Fail();
            return false;
        }

        m_impl->m_inputValues.push_back(&m_impl->m_document["$value"]);
        return true;
    }


    void JsonContext::EndDocument()
    {
        if (!IsSerializing() || !IsValid())
            return;

        m_impl->m_writer->EndObject();
        const size_t size = m_impl->m_output.GetSize();
        if (m_impl->m_stream->WriteFromBuffer(m_impl->m_output.GetString(), size) != size)
            Fail();
    }


    bool JsonContext::BeginObjectImpl()
    {
        if (IsSerializing())
        {
            m_impl->m_writer->StartObject();
            return true;
        }

        if (!m_impl->m_inputValues.back()->IsObject())
        {
            Fail();
            return false;
        }
        return true;
    }


    void JsonContext::EndObjectImpl()
    {
        if (IsSerializing())
            m_impl->m_writer->EndObject();
    }


    bool JsonContext::BeginField(const festd::ascii_view name, uint64_t)
    {
        if (IsSerializing())
        {
            m_impl->m_writer->Key(name.data(), static_cast<rapidjson::SizeType>(name.size()));
            return true;
        }

        rapidjson::Value* object = m_impl->m_inputValues.back();
        const auto member = object->FindMember(rapidjson::StringRef(name.data(), name.size()));
        if (member == object->MemberEnd())
            return false;

        m_impl->m_inputValues.push_back(&member->value);
        return true;
    }


    void JsonContext::EndField()
    {
        if (IsDeserializing())
            m_impl->m_inputValues.pop_back();
    }


    bool JsonContext::BeginArrayImpl(uint32_t& size)
    {
        if (IsSerializing())
        {
            m_impl->m_writer->StartArray();
            return true;
        }

        rapidjson::Value* array = m_impl->m_inputValues.back();
        if (!array->IsArray())
        {
            Fail();
            return false;
        }

        size = array->Size();
        return true;
    }


    void JsonContext::EndArrayImpl()
    {
        if (IsSerializing())
            m_impl->m_writer->EndArray();
    }


    bool JsonContext::BeginElement(const uint32_t index)
    {
        if (IsSerializing())
            return true;

        rapidjson::Value* array = m_impl->m_inputValues.back();
        if (!array->IsArray() || index >= array->Size())
        {
            Fail();
            return false;
        }

        m_impl->m_inputValues.push_back(&(*array)[index]);
        return true;
    }


    void JsonContext::EndElement()
    {
        if (IsDeserializing())
            m_impl->m_inputValues.pop_back();
    }


    void JsonContext::TransferScalar(const ScalarKind kind, void* value, const uint32_t byteSize)
    {
        if (IsSerializing())
        {
            auto& writer = *m_impl->m_writer;
            switch (kind)
            {
            case ScalarKind::kBool:
                writer.Bool(*static_cast<const bool*>(value));
                return;
            case ScalarKind::kSigned:
                writer.Int64(LoadSigned(value, byteSize));
                return;
            case ScalarKind::kUnsigned:
                writer.Uint64(LoadUnsigned(value, byteSize));
                return;
            case ScalarKind::kFloat:
                {
                    char buffer[64];
                    const double number = byteSize == sizeof(float) ? static_cast<double>(*static_cast<const float*>(value))
                                                                    : *static_cast<const double*>(value);
                    if (!std::isfinite(number))
                    {
                        Fail();
                        return;
                    }

                    char* numberBegin = buffer + 2;
                    double magnitude = number;
                    if (std::signbit(number))
                    {
                        buffer[0] = '-';
                        buffer[1] = '0';
                        buffer[2] = 'x';
                        numberBegin = buffer + 3;
                        magnitude = -number;
                    }
                    else
                    {
                        buffer[0] = '0';
                        buffer[1] = 'x';
                    }

                    const auto result = std::to_chars(numberBegin, buffer + sizeof(buffer), magnitude, std::chars_format::hex);
                    if (result.ec != std::errc{})
                    {
                        Fail();
                        return;
                    }

                    writer.String(buffer, static_cast<rapidjson::SizeType>(result.ptr - buffer));
                    return;
                }
            }
        }

        const rapidjson::Value* input = m_impl->m_inputValues.back();
        switch (kind)
        {
        case ScalarKind::kBool:
            if (!input->IsBool())
                Fail();
            else
                *static_cast<bool*>(value) = input->GetBool();
            return;
        case ScalarKind::kSigned:
            if (!input->IsInt64() || !StoreSigned(value, byteSize, input->GetInt64()))
                Fail();
            return;
        case ScalarKind::kUnsigned:
            if (!input->IsUint64() || !StoreUnsigned(value, byteSize, input->GetUint64()))
                Fail();
            return;
        case ScalarKind::kFloat:
            {
                if (input->IsNumber())
                {
                    if (byteSize == sizeof(float))
                        *static_cast<float*>(value) = input->GetFloat();
                    else
                        *static_cast<double*>(value) = input->GetDouble();
                    return;
                }

                if (!input->IsString())
                {
                    Fail();
                    return;
                }

                char* parseEnd = nullptr;
                if (byteSize == sizeof(float))
                    *static_cast<float*>(value) = std::strtof(input->GetString(), &parseEnd);
                else
                    *static_cast<double*>(value) = std::strtod(input->GetString(), &parseEnd);

                if (parseEnd != input->GetString() + input->GetStringLength())
                    Fail();
                return;
            }
        }
    }


    void JsonContext::TransferBytes(void* value, const uint32_t byteSize)
    {
        static constexpr char kHex[] = "0123456789abcdef";
        if (IsSerializing())
        {
            festd::vector<char> buffer;
            buffer.resize(byteSize * 2);
            const auto* bytes = static_cast<const uint8_t*>(value);
            for (uint32_t i = 0; i < byteSize; ++i)
            {
                buffer[i * 2] = kHex[bytes[i] >> 4];
                buffer[i * 2 + 1] = kHex[bytes[i] & 0xf];
            }
            m_impl->m_writer->String(buffer.data(), buffer.size());
            return;
        }

        const rapidjson::Value* input = m_impl->m_inputValues.back();
        if (!input->IsString() || input->GetStringLength() != byteSize * 2)
        {
            Fail();
            return;
        }

        auto HexValue = [](const char c) -> uint8_t {
            if (c >= '0' && c <= '9')
                return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f')
                return static_cast<uint8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F')
                return static_cast<uint8_t>(c - 'A' + 10);
            return UINT8_MAX;
        };

        auto* bytes = static_cast<uint8_t*>(value);
        for (uint32_t i = 0; i < byteSize; ++i)
        {
            const uint8_t high = HexValue(input->GetString()[i * 2]);
            const uint8_t low = HexValue(input->GetString()[i * 2 + 1]);
            if (high == UINT8_MAX || low == UINT8_MAX)
            {
                Fail();
                return;
            }
            bytes[i] = static_cast<uint8_t>((high << 4) | low);
        }
    }


    void JsonContext::TransferString(festd::string* output, const festd::string_view input)
    {
        if (IsSerializing())
        {
            const char* data = input.empty() ? "" : input.data();
            m_impl->m_writer->String(data, static_cast<rapidjson::SizeType>(input.size()));
            return;
        }

        const rapidjson::Value* value = m_impl->m_inputValues.back();
        if (!value->IsString())
        {
            Fail();
            return;
        }
        output->assign(value->GetString(), value->GetStringLength());
    }
} // namespace FE::Serialization
