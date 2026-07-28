#include <Core/Serialization/JsonSerialization.h>
#include <Core/Strings/Format.h>
#include <Core/Strings/Parser.h>
#include <cmath>
FE_PUSH_MSVC_WARNING(5054)
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
FE_POP_MSVC_WARNING()

namespace FE::Serialization
{
    namespace
    {
        struct RapidJsonAllocator final
        {
            static constexpr bool kNeedFree = true;

            void* Malloc(const size_t size)
            {
                return size == 0 ? nullptr : Memory::DefaultAllocate(size);
            }

            void* Realloc(void* original, size_t, const size_t newSize)
            {
                if (newSize == 0)
                {
                    if (original != nullptr)
                        Memory::DefaultFree(original);
                    return nullptr;
                }
                if (original == nullptr)
                    return Memory::DefaultAllocate(newSize);
                return Memory::DefaultReallocate(original, newSize);
            }

            static void Free(void* pointer)
            {
                if (pointer != nullptr)
                    Memory::DefaultFree(pointer);
            }
        };


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


        void GetLineAndColumn(const festd::span<const char> input, const uint64_t offset, uint32_t& line, uint32_t& column)
        {
            line = 1;
            column = 1;
            const uint32_t end = static_cast<uint32_t>(std::min<uint64_t>(offset, input.size()));
            for (uint32_t i = 0; i < end; ++i)
            {
                if (input[i] == '\n')
                {
                    ++line;
                    column = 1;
                }
                else
                {
                    ++column;
                }
            }
        }
    } // namespace


    struct JsonContext::Impl final
    {
        using Encoding = rapidjson::UTF8<>;
        using PoolAllocator = rapidjson::MemoryPoolAllocator<RapidJsonAllocator>;
        using Output = rapidjson::GenericStringBuffer<Encoding, RapidJsonAllocator>;
        using Writer = rapidjson::PrettyWriter<Output, Encoding, Encoding, RapidJsonAllocator>;
        using Document = rapidjson::GenericDocument<Encoding, PoolAllocator, RapidJsonAllocator>;
        using Value = rapidjson::GenericValue<Encoding, PoolAllocator>;

        explicit Impl(IO::IStream* stream)
            : m_stream(stream)
            , m_poolAllocator(64 * 1024, &m_allocator)
            , m_output(&m_allocator)
            , m_writer(m_output, &m_allocator)
            , m_document(&m_poolAllocator, 1024, &m_allocator)
        {
        }

        IO::IStream* m_stream;
        RapidJsonAllocator m_allocator;
        PoolAllocator m_poolAllocator;
        Output m_output;
        Writer m_writer;
        Document m_document;
        festd::vector<Value*> m_inputValues;
        festd::vector<char> m_input;
        uint32_t m_pendingStringSize = 0;
    };


    JsonContext::JsonContext(IO::IStream* stream)
        : SerializationContext(Format::kJson)
        , m_impl(festd::make_unique<Impl>(stream))
    {
        FE_Assert(stream != nullptr);
    }


    JsonContext::~JsonContext() = default;


    void JsonContext::Reset()
    {
        m_impl->m_document.SetNull();
        m_impl->m_poolAllocator.Clear();
        m_impl->m_output.Clear();
        m_impl->m_writer.Reset(m_impl->m_output);
        m_impl->m_inputValues.clear();
        m_impl->m_input.clear();
        m_impl->m_pendingStringSize = 0;
    }


    bool JsonContext::BeginDocument(const Rtti::TypeID expectedType, const uint32_t version, const uint64_t schemaHash)
    {
        if (IsSerializing())
        {
            auto& writer = m_impl->m_writer;
            writer.StartObject();

            const festd::fixed_string typeString = Fmt::FixedFormat("{}", expectedType);
            writer.Key("$type");
            writer.String(typeString.data(), typeString.size());
            writer.Key("$version");
            writer.Uint(version);

            festd::basic_fixed_string<18> schemaString = "0x";
            schemaString.append(Fmt::HexFormatter{ schemaHash }.View());
            writer.Key("$schema");
            writer.String(schemaString.data(), schemaString.size());
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

        m_impl->m_document.ParseInsitu(m_impl->m_input.data());
        if (m_impl->m_document.HasParseError())
        {
            const uint64_t offset = m_impl->m_document.GetErrorOffset();
            uint32_t line = 0;
            uint32_t column = 0;
            GetLineAndColumn(festd::span<const char>{ m_impl->m_input.data(), m_impl->m_input.size() }, offset, line, column);
            Fail(ErrorCode::kJsonParseError, offset, line, column);
            return false;
        }
        if (!m_impl->m_document.IsObject())
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (!m_impl->m_document.HasMember("$type"))
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (!m_impl->m_document.HasMember("$version"))
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (!m_impl->m_document.HasMember("$schema"))
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (!m_impl->m_document.HasMember("$value"))
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }

        const auto& typeValue = m_impl->m_document["$type"];
        const auto& versionValue = m_impl->m_document["$version"];
        const auto& schemaValue = m_impl->m_document["$schema"];
        if (!typeValue.IsString())
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (!versionValue.IsUint())
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (!schemaValue.IsString())
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }

        const Rtti::TypeID serializedType =
            Rtti::TypeID::Parse(festd::ascii_view{ typeValue.GetString(), typeValue.GetStringLength() });
        if (expectedType.IsValid() && serializedType != expectedType)
        {
            Fail(ErrorCode::kTypeMismatch);
            return false;
        }

        festd::string_view schemaString{ schemaValue.GetString(), schemaValue.GetStringLength() };
        if (schemaString.starts_with("0x"))
            schemaString = schemaString.substr(2);
        if (Parser::TryParseUInt64(schemaString, m_serializedSchemaHash, 16) != Parser::ResultCode::kSuccess)
        {
            Fail(ErrorCode::kInvalidNumber);
            return false;
        }

        m_serializedVersion = versionValue.GetUint();
        m_impl->m_inputValues.push_back(&m_impl->m_document["$value"]);
        return true;
    }


    void JsonContext::EndDocument()
    {
        if (!IsSerializing() || !IsValid())
            return;

        m_impl->m_writer.EndObject();
        const size_t size = m_impl->m_output.GetSize();
        if (m_impl->m_stream->WriteFromBuffer(m_impl->m_output.GetString(), size) != size)
            Fail(ErrorCode::kStreamWriteFailed);
    }


    bool JsonContext::BeginObjectImpl()
    {
        if (IsSerializing())
        {
            m_impl->m_writer.StartObject();
            return true;
        }

        if (!m_impl->m_inputValues.back()->IsObject())
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        return true;
    }


    void JsonContext::EndObjectImpl()
    {
        if (IsSerializing() && IsValid())
            m_impl->m_writer.EndObject();
    }


    bool JsonContext::BeginField(const festd::ascii_view name, uint64_t)
    {
        if (IsSerializing())
        {
            m_impl->m_writer.Key(name.data(), static_cast<rapidjson::SizeType>(name.size()));
            return true;
        }

        Impl::Value* object = m_impl->m_inputValues.back();
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
            m_impl->m_writer.StartArray();
            return true;
        }

        Impl::Value* array = m_impl->m_inputValues.back();
        if (!array->IsArray())
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }

        size = array->Size();
        return true;
    }


    void JsonContext::EndArrayImpl()
    {
        if (IsSerializing() && IsValid())
            m_impl->m_writer.EndArray();
    }


    bool JsonContext::BeginElement(const uint32_t index)
    {
        if (IsSerializing())
            return true;

        Impl::Value* array = m_impl->m_inputValues.back();
        if (!array->IsArray())
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (index >= array->Size())
        {
            Fail(ErrorCode::kMalformedData);
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


    void JsonContext::StoreScalarImpl(const ScalarKind kind, const void* value, const uint32_t byteSize)
    {
        auto& writer = m_impl->m_writer;
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
            if (byteSize == sizeof(float))
            {
                const float number = *static_cast<const float*>(value);
                if (!std::isfinite(number))
                {
                    Fail(ErrorCode::kUnsupportedValue);
                    return;
                }
                const Fmt::HexFloatFormatter formatter{ number };
                writer.String(formatter.View().data(), formatter.View().size());
                return;
            }

            const double number = *static_cast<const double*>(value);
            if (!std::isfinite(number))
            {
                Fail(ErrorCode::kUnsupportedValue);
                return;
            }
            const Fmt::HexFloatFormatter formatter{ number };
            writer.String(formatter.View().data(), formatter.View().size());
            return;
        }
    }


    void JsonContext::LoadScalarImpl(const ScalarKind kind, void* value, const uint32_t byteSize)
    {
        const Impl::Value* input = m_impl->m_inputValues.back();
        switch (kind)
        {
        case ScalarKind::kBool:
            if (!input->IsBool())
                Fail(ErrorCode::kMalformedData);
            else
                *static_cast<bool*>(value) = input->GetBool();
            return;
        case ScalarKind::kSigned:
            if (!input->IsInt64())
            {
                Fail(ErrorCode::kMalformedData);
                return;
            }
            if (!StoreSigned(value, byteSize, input->GetInt64()))
                Fail(ErrorCode::kInvalidNumber);
            return;
        case ScalarKind::kUnsigned:
            if (!input->IsUint64())
            {
                Fail(ErrorCode::kMalformedData);
                return;
            }
            if (!StoreUnsigned(value, byteSize, input->GetUint64()))
                Fail(ErrorCode::kInvalidNumber);
            return;
        case ScalarKind::kFloat:
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
                Fail(ErrorCode::kMalformedData);
                return;
            }

            double parsedValue = 0.0;
            const festd::string_view string{ input->GetString(), input->GetStringLength() };
            if (Parser::TryParseHexDouble(string, parsedValue) != Parser::ResultCode::kSuccess)
            {
                Fail(ErrorCode::kInvalidNumber);
                return;
            }
            if (byteSize == sizeof(float))
                *static_cast<float*>(value) = static_cast<float>(parsedValue);
            else
                *static_cast<double*>(value) = parsedValue;
            return;
        }
    }


    void JsonContext::StoreBytesImpl(const void* value, const uint32_t byteSize)
    {
        static constexpr char kHex[] = "0123456789abcdef";
        festd::inline_string buffer;
        buffer.resize_uninitialized(byteSize * 2);
        const auto* bytes = static_cast<const uint8_t*>(value);
        for (uint32_t i = 0; i < byteSize; ++i)
        {
            buffer.data()[i * 2] = kHex[bytes[i] >> 4];
            buffer.data()[i * 2 + 1] = kHex[bytes[i] & 0xf];
        }
        m_impl->m_writer.String(buffer.data(), buffer.size());
    }


    void JsonContext::LoadBytesImpl(void* value, const uint32_t byteSize)
    {
        const Impl::Value* input = m_impl->m_inputValues.back();
        if (!input->IsString())
        {
            Fail(ErrorCode::kMalformedData);
            return;
        }
        if (input->GetStringLength() != byteSize * 2)
        {
            Fail(ErrorCode::kMalformedData);
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
                Fail(ErrorCode::kInvalidString);
                return;
            }
            bytes[i] = static_cast<uint8_t>((high << 4) | low);
        }
    }


    void JsonContext::StoreStringImpl(const festd::string_view value)
    {
        const char* data = value.empty() ? "" : value.data();
        m_impl->m_writer.String(data, static_cast<rapidjson::SizeType>(value.size()));
    }


    uint32_t JsonContext::LoadStringSizeImpl()
    {
        const Impl::Value* value = m_impl->m_inputValues.back();
        if (!value->IsString())
        {
            Fail(ErrorCode::kMalformedData);
            return 0;
        }
        m_impl->m_pendingStringSize = value->GetStringLength();
        return m_impl->m_pendingStringSize;
    }


    void JsonContext::LoadStringImpl(const festd::span<char> buffer)
    {
        if (buffer.size() != m_impl->m_pendingStringSize)
        {
            Fail(ErrorCode::kMalformedData);
            return;
        }

        const Impl::Value* value = m_impl->m_inputValues.back();
        if (!buffer.empty())
            memcpy(buffer.data(), value->GetString(), buffer.size());
        m_impl->m_pendingStringSize = 0;
    }


    uint64_t JsonContext::GetCurrentOffset() const
    {
        return m_impl->m_stream->Tell();
    }
} // namespace FE::Serialization
