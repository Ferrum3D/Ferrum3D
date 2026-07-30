#include <Core/Serialization/JsonSerialization.h>
#include <Core/Strings/Format.h>
#include <Core/Strings/Parser.h>

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
            const uint32_t end = static_cast<uint32_t>(Math::Min<uint64_t>(offset, input.size()));
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


    struct JsonFormat::Impl final
    {
        using Encoding = rapidjson::UTF8<>;
        using PoolAllocator = rapidjson::MemoryPoolAllocator<RapidJsonAllocator>;
        using Output = rapidjson::GenericStringBuffer<Encoding, RapidJsonAllocator>;
        using Writer = rapidjson::PrettyWriter<Output, Encoding, Encoding, RapidJsonAllocator>;
        using Document = rapidjson::GenericDocument<Encoding, PoolAllocator, RapidJsonAllocator>;
        using Value = rapidjson::GenericValue<Encoding, PoolAllocator>;

        Impl()
            : m_poolAllocator(64 * 1024, &m_allocator)
            , m_output(&m_allocator)
            , m_writer(m_output, &m_allocator)
            , m_document(&m_poolAllocator, 1024, &m_allocator)
        {
        }

        RapidJsonAllocator m_allocator;
        PoolAllocator m_poolAllocator;
        Output m_output;
        Writer m_writer;
        Document m_document;
        festd::vector<Value*> m_inputValues;
        festd::vector<char> m_input;
        uint32_t m_pendingStringSize = 0;
    };


    JsonFormat::JsonFormat()
        : SerializationFormat(Format::kJson)
        , m_impl(festd::make_unique<Impl>())
    {
    }


    JsonFormat::~JsonFormat() = default;


    void JsonFormat::ResetImpl()
    {
        m_impl->m_document.SetNull();
        m_impl->m_poolAllocator.Clear();
        m_impl->m_output.Clear();
        m_impl->m_writer.Reset(m_impl->m_output);
        m_impl->m_inputValues.clear();
        m_impl->m_input.clear();
        m_impl->m_pendingStringSize = 0;
    }


    ResultCode JsonFormat::BeginDocumentImpl(const Rtti::TypeID expectedType, const uint32_t version, const uint64_t schemaHash)
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
            return ResultCode::kSuccess;
        }

        char chunk[4096];
        while (true)
        {
            const size_t bytesRead = m_stream->ReadToBuffer(chunk, sizeof(chunk));
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
            GetLineAndColumn(m_impl->m_input, offset, line, column);
            return Fail(ResultCode::kJsonParseError, offset, line, column);
        }

        if (!m_impl->m_document.IsObject())
            return Fail(ResultCode::kMalformedData);

        if (!m_impl->m_document.HasMember("$type"))
            return Fail(ResultCode::kMalformedData);

        if (!m_impl->m_document.HasMember("$version"))
            return Fail(ResultCode::kMalformedData);

        if (!m_impl->m_document.HasMember("$schema"))
            return Fail(ResultCode::kMalformedData);

        if (!m_impl->m_document.HasMember("$value"))
            return Fail(ResultCode::kMalformedData);

        const auto& typeValue = m_impl->m_document["$type"];
        if (!typeValue.IsString())
            return Fail(ResultCode::kMalformedData);

        const auto& versionValue = m_impl->m_document["$version"];
        if (!versionValue.IsUint())
            return Fail(ResultCode::kMalformedData);

        const auto& schemaValue = m_impl->m_document["$schema"];
        if (!schemaValue.IsString())
            return Fail(ResultCode::kMalformedData);

        const festd::ascii_view serializedTypeIdString{ typeValue.GetString(), typeValue.GetStringLength() };
        const Rtti::TypeID serializedType = Rtti::TypeID::Parse(serializedTypeIdString);
        if (expectedType.IsValid() && serializedType != expectedType)
            return Fail(ResultCode::kTypeMismatch);

        uint64_t serializedSchemaHash = 0;
        const festd::string_view schemaString{ schemaValue.GetString(), schemaValue.GetStringLength() };
        if (Parser::TryParseUInt64(schemaString, serializedSchemaHash, 16) != Parser::ResultCode::kSuccess)
            return Fail(ResultCode::kInvalidNumber);

        m_serializedVersion = versionValue.GetUint();
        m_serializedSchemaHash = serializedSchemaHash;
        m_impl->m_inputValues.push_back(&m_impl->m_document["$value"]);
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::EndDocumentImpl()
    {
        if (!IsSerializing() || !IsValid())
            return m_error.m_code;

        m_impl->m_writer.EndObject();
        const size_t size = m_impl->m_output.GetSize();
        if (m_stream->WriteFromBuffer(m_impl->m_output.GetString(), size) != size)
            return Fail(ResultCode::kStreamWriteFailed);

        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::BeginObjectImpl()
    {
        if (IsSerializing())
        {
            m_impl->m_writer.StartObject();
            return ResultCode::kSuccess;
        }

        if (!m_impl->m_inputValues.back()->IsObject())
            return Fail(ResultCode::kMalformedData);

        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::EndObjectImpl()
    {
        if (IsSerializing() && IsValid())
            m_impl->m_writer.EndObject();

        return m_error.m_code;
    }


    ResultCode JsonFormat::BeginFieldImpl(const festd::ascii_view name, uint64_t, bool& exists)
    {
        exists = true;
        if (IsSerializing())
        {
            m_impl->m_writer.Key(name.data(), static_cast<rapidjson::SizeType>(name.size()));
            return ResultCode::kSuccess;
        }

        Impl::Value* object = m_impl->m_inputValues.back();
        const auto member = object->FindMember(rapidjson::StringRef(name.data(), name.size()));
        if (member == object->MemberEnd())
        {
            exists = false;
            return ResultCode::kSuccess;
        }

        m_impl->m_inputValues.push_back(&member->value);
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::EndFieldImpl()
    {
        if (IsDeserializing())
            m_impl->m_inputValues.pop_back();

        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::BeginArrayImpl(uint32_t& size)
    {
        if (IsSerializing())
        {
            m_impl->m_writer.StartArray();
            return ResultCode::kSuccess;
        }

        Impl::Value* array = m_impl->m_inputValues.back();
        if (!array->IsArray())
            return Fail(ResultCode::kMalformedData);

        size = array->Size();
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::EndArrayImpl()
    {
        if (IsSerializing() && IsValid())
            m_impl->m_writer.EndArray();

        return m_error.m_code;
    }


    ResultCode JsonFormat::BeginElementImpl(const uint32_t index)
    {
        if (IsSerializing())
            return ResultCode::kSuccess;

        Impl::Value* array = m_impl->m_inputValues.back();
        if (!array->IsArray())
            return Fail(ResultCode::kMalformedData);

        if (index >= array->Size())
            return Fail(ResultCode::kMalformedData);

        m_impl->m_inputValues.push_back(&(*array)[index]);
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::EndElementImpl()
    {
        if (IsDeserializing())
            m_impl->m_inputValues.pop_back();
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::StoreScalarImpl(const ScalarKind kind, const void* value, const uint32_t byteSize)
    {
        auto& writer = m_impl->m_writer;
        switch (kind)
        {
        case ScalarKind::kBool:
            writer.Bool(*static_cast<const bool*>(value));
            return ResultCode::kSuccess;

        case ScalarKind::kSigned:
            writer.Int64(LoadSigned(value, byteSize));
            return ResultCode::kSuccess;

        case ScalarKind::kUnsigned:
            writer.Uint64(LoadUnsigned(value, byteSize));
            return ResultCode::kSuccess;

        case ScalarKind::kFloat:
            if (byteSize == sizeof(float))
            {
                const float number = *static_cast<const float*>(value);
                const uint32_t bits = std::bit_cast<uint32_t>(number);
                writer.Uint(bits);
                return ResultCode::kSuccess;
            }

            const double number = *static_cast<const double*>(value);
            const uint64_t bits = std::bit_cast<uint64_t>(number);
            writer.Uint64(bits);
            return ResultCode::kSuccess;
        }

        return Fail(ResultCode::kUnsupportedValue);
    }


    ResultCode JsonFormat::LoadScalarImpl(const ScalarKind kind, void* value, const uint32_t byteSize)
    {
        const Impl::Value* input = m_impl->m_inputValues.back();
        switch (kind)
        {
        case ScalarKind::kBool:
            if (!input->IsBool())
                return Fail(ResultCode::kMalformedData);
            *static_cast<bool*>(value) = input->GetBool();
            return ResultCode::kSuccess;

        case ScalarKind::kSigned:
            if (!input->IsInt64())
                return Fail(ResultCode::kMalformedData);

            if (!StoreSigned(value, byteSize, input->GetInt64()))
                return Fail(ResultCode::kInvalidNumber);

            return ResultCode::kSuccess;

        case ScalarKind::kUnsigned:
            if (!input->IsUint64())
                return Fail(ResultCode::kMalformedData);

            if (!StoreUnsigned(value, byteSize, input->GetUint64()))
                return Fail(ResultCode::kInvalidNumber);

            return ResultCode::kSuccess;

        case ScalarKind::kFloat:
            if (input->IsNumber())
            {
                if (byteSize == sizeof(float))
                {
                    const uint32_t bits = input->GetUint();
                    *static_cast<float*>(value) = std::bit_cast<float>(bits);
                }
                else
                {
                    const uint64_t bits = input->GetUint64();
                    *static_cast<double*>(value) = std::bit_cast<double>(bits);
                }

                return ResultCode::kSuccess;
            }

            return Fail(ResultCode::kMalformedData);
        }

        return Fail(ResultCode::kUnsupportedValue);
    }


    ResultCode JsonFormat::StoreBytesImpl(const void* value, const uint32_t byteSize)
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
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::LoadBytesImpl(void* value, const uint32_t byteSize)
    {
        const Impl::Value* input = m_impl->m_inputValues.back();
        if (!input->IsString())
            return Fail(ResultCode::kMalformedData);

        if (input->GetStringLength() != byteSize * 2)
            return Fail(ResultCode::kMalformedData);

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
                return Fail(ResultCode::kInvalidString);

            bytes[i] = static_cast<uint8_t>((high << 4) | low);
        }

        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::StoreStringImpl(const festd::string_view value)
    {
        const char* data = value.empty() ? "" : value.data();
        m_impl->m_writer.String(data, value.size());
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::LoadStringSizeImpl(uint32_t& size)
    {
        const Impl::Value* value = m_impl->m_inputValues.back();
        if (!value->IsString())
            return Fail(ResultCode::kMalformedData);

        m_impl->m_pendingStringSize = value->GetStringLength();
        size = m_impl->m_pendingStringSize;
        return ResultCode::kSuccess;
    }


    ResultCode JsonFormat::LoadStringImpl(const festd::span<char> buffer)
    {
        if (buffer.size() != m_impl->m_pendingStringSize)
            return Fail(ResultCode::kMalformedData);

        const Impl::Value* value = m_impl->m_inputValues.back();
        if (!buffer.empty())
            memcpy(buffer.data(), value->GetString(), buffer.size());

        m_impl->m_pendingStringSize = 0;
        return ResultCode::kSuccess;
    }


    uint64_t JsonFormat::GetCurrentOffsetImpl() const
    {
        return m_stream->Tell();
    }
} // namespace FE::Serialization
