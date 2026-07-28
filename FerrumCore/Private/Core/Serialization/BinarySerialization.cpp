#include <Core/Serialization/BinarySerialization.h>

namespace FE::Serialization
{
    namespace
    {
        inline constexpr uint32_t kPackedMagic = UINT32_C(0x4b505346);
        inline constexpr uint32_t kTaggedMagic = UINT32_C(0x47545346);


        struct PackedHeader final
        {
            uint64_t m_schemaHash;
            uint32_t m_magic;
            uint32_t m_version;
            uint8_t m_typeID[16];
        };


        struct TaggedHeader final
        {
            uint64_t m_schemaHash;
            uint64_t m_payloadSize;
            uint32_t m_magic;
            uint32_t m_version;
            uint8_t m_typeID[16];
        };


        static_assert(sizeof(PackedHeader) == 32);
        static_assert(sizeof(TaggedHeader) == 40);
        static_assert(std::is_trivially_copyable_v<PackedHeader>);
        static_assert(std::is_trivially_copyable_v<TaggedHeader>);


        PackedHeader CreatePackedHeader(const Rtti::TypeID type, const uint32_t version, const uint64_t schemaHash)
        {
            PackedHeader header{};
            header.m_schemaHash = schemaHash;
            header.m_magic = kPackedMagic;
            header.m_version = version;
            memcpy(header.m_typeID, type.data(), type.size());
            return header;
        }


        TaggedHeader CreateTaggedHeader(const Rtti::TypeID type, const uint32_t version, const uint64_t schemaHash)
        {
            TaggedHeader header{};
            header.m_schemaHash = schemaHash;
            header.m_magic = kTaggedMagic;
            header.m_version = version;
            memcpy(header.m_typeID, type.data(), type.size());
            return header;
        }


        void AppendBytes(festd::vector<std::byte>& destination, const void* data, const size_t size)
        {
            if (size == 0)
                return;

            const auto* first = static_cast<const std::byte*>(data);
            destination.insert(destination.end(), first, first + size);
        }


        template<class T>
        void Append(festd::vector<std::byte>& destination, const T& value)
        {
            AppendBytes(destination, &value, sizeof(value));
        }


        struct InputFrame final
        {
            const std::byte* m_data = nullptr;
            size_t m_size = 0;
            size_t m_position = 0;
        };


        bool ReadFrame(InputFrame& frame, void* destination, const size_t size)
        {
            if (frame.m_position > frame.m_size)
                return false;
            if (size > frame.m_size - frame.m_position)
                return false;

            if (size != 0)
                memcpy(destination, frame.m_data + frame.m_position, size);
            frame.m_position += size;
            return true;
        }
    } // namespace


    struct PackedBinaryContext::Impl final
    {
        explicit Impl(IO::IStream* stream)
            : m_stream(stream)
        {
        }

        IO::IStream* m_stream;
        uint32_t m_pendingStringSize = 0;
    };


    PackedBinaryContext::PackedBinaryContext(IO::IStream* stream)
        : SerializationContext(Format::kPackedBinary)
        , m_impl(festd::make_unique<Impl>(stream))
    {
        FE_Assert(stream != nullptr);
    }


    PackedBinaryContext::~PackedBinaryContext() = default;


    void PackedBinaryContext::Reset()
    {
        m_impl->m_pendingStringSize = 0;
    }


    bool PackedBinaryContext::BeginDocument(const Rtti::TypeID expectedType, const uint32_t version, const uint64_t schemaHash)
    {
        if (IsSerializing())
        {
            const PackedHeader header = CreatePackedHeader(expectedType, version, schemaHash);
            if (!m_impl->m_stream->Write(header))
            {
                Fail(ErrorCode::kStreamWriteFailed);
                return false;
            }
            return true;
        }

        PackedHeader header{};
        if (!m_impl->m_stream->Read(header))
        {
            Fail(ErrorCode::kStreamReadFailed);
            return false;
        }
        if (header.m_magic != kPackedMagic)
        {
            Fail(ErrorCode::kInvalidHeader);
            return false;
        }

        const Rtti::TypeID serializedType = Rtti::TypeID::LoadUnaligned(header.m_typeID);
        if (expectedType.IsValid() && serializedType != expectedType)
        {
            Fail(ErrorCode::kTypeMismatch);
            return false;
        }
        if (schemaHash != 0 && header.m_schemaHash != schemaHash)
        {
            Fail(ErrorCode::kSchemaMismatch);
            return false;
        }

        m_serializedVersion = header.m_version;
        m_serializedSchemaHash = header.m_schemaHash;
        return true;
    }


    void PackedBinaryContext::EndDocument() {}
    bool PackedBinaryContext::BeginObjectImpl()
    {
        return true;
    }
    void PackedBinaryContext::EndObjectImpl() {}
    bool PackedBinaryContext::BeginField(festd::ascii_view, uint64_t)
    {
        return true;
    }
    void PackedBinaryContext::EndField() {}


    bool PackedBinaryContext::BeginArrayImpl(uint32_t& size)
    {
        const bool succeeded = IsSerializing() ? m_impl->m_stream->Write(size) : m_impl->m_stream->Read(size);
        if (!succeeded)
            Fail(IsSerializing() ? ErrorCode::kStreamWriteFailed : ErrorCode::kStreamReadFailed);
        return IsValid();
    }


    void PackedBinaryContext::EndArrayImpl() {}
    bool PackedBinaryContext::BeginElement(uint32_t)
    {
        return true;
    }
    void PackedBinaryContext::EndElement() {}


    void PackedBinaryContext::StoreScalarImpl(ScalarKind, const void* value, const uint32_t byteSize)
    {
        StoreBytesImpl(value, byteSize);
    }


    void PackedBinaryContext::LoadScalarImpl(ScalarKind, void* value, const uint32_t byteSize)
    {
        LoadBytesImpl(value, byteSize);
    }


    void PackedBinaryContext::StoreBytesImpl(const void* value, const uint32_t byteSize)
    {
        if (m_impl->m_stream->WriteFromBuffer(value, byteSize) != byteSize)
            Fail(ErrorCode::kStreamWriteFailed);
    }


    void PackedBinaryContext::LoadBytesImpl(void* value, const uint32_t byteSize)
    {
        if (m_impl->m_stream->ReadToBuffer(value, byteSize) != byteSize)
            Fail(ErrorCode::kStreamReadFailed);
    }


    void PackedBinaryContext::StoreStringImpl(const festd::string_view value)
    {
        const uint32_t size = value.size();
        if (!m_impl->m_stream->Write(size))
        {
            Fail(ErrorCode::kStreamWriteFailed);
            return;
        }
        if (size != 0 && m_impl->m_stream->WriteFromBuffer(value.data(), size) != size)
            Fail(ErrorCode::kStreamWriteFailed);
    }


    uint32_t PackedBinaryContext::LoadStringSizeImpl()
    {
        if (!m_impl->m_stream->Read(m_impl->m_pendingStringSize))
            Fail(ErrorCode::kStreamReadFailed);
        return m_impl->m_pendingStringSize;
    }


    void PackedBinaryContext::LoadStringImpl(const festd::span<char> buffer)
    {
        if (buffer.size() != m_impl->m_pendingStringSize)
        {
            Fail(ErrorCode::kMalformedData);
            return;
        }
        if (!buffer.empty() && m_impl->m_stream->ReadToBuffer(buffer.data(), buffer.size()) != buffer.size())
            Fail(ErrorCode::kStreamReadFailed);
        m_impl->m_pendingStringSize = 0;
    }


    uint64_t PackedBinaryContext::GetCurrentOffset() const
    {
        return m_impl->m_stream->Tell();
    }


    struct TaggedBinaryContext::Impl final
    {
        struct InputArray final
        {
            InputFrame m_frame;
            uint32_t m_count = 0;
        };

        explicit Impl(IO::IStream* stream)
            : m_stream(stream)
        {
        }

        IO::IStream* m_stream;
        TaggedHeader m_outputHeader{};
        festd::vector<festd::vector<std::byte>> m_outputFrames;
        festd::vector<uint64_t> m_outputFieldIDs;
        festd::vector<InputFrame> m_inputFrames;
        festd::vector<InputArray> m_inputArrays;
        festd::vector<std::byte> m_inputStorage;
        uint32_t m_pendingStringSize = 0;
    };


    TaggedBinaryContext::TaggedBinaryContext(IO::IStream* stream)
        : SerializationContext(Format::kTaggedBinary)
        , m_impl(festd::make_unique<Impl>(stream))
    {
        FE_Assert(stream != nullptr);
    }


    TaggedBinaryContext::~TaggedBinaryContext() = default;


    void TaggedBinaryContext::Reset()
    {
        m_impl->m_outputHeader = {};
        m_impl->m_outputFrames.clear();
        m_impl->m_outputFieldIDs.clear();
        m_impl->m_inputFrames.clear();
        m_impl->m_inputArrays.clear();
        m_impl->m_inputStorage.clear();
        m_impl->m_pendingStringSize = 0;
    }


    bool TaggedBinaryContext::BeginDocument(const Rtti::TypeID expectedType, const uint32_t version, const uint64_t schemaHash)
    {
        if (IsSerializing())
        {
            m_impl->m_outputHeader = CreateTaggedHeader(expectedType, version, schemaHash);
            m_impl->m_outputFrames.emplace_back();
            return true;
        }

        TaggedHeader header{};
        if (!m_impl->m_stream->Read(header))
        {
            Fail(ErrorCode::kStreamReadFailed);
            return false;
        }
        if (header.m_magic != kTaggedMagic)
        {
            Fail(ErrorCode::kInvalidHeader);
            return false;
        }

        const Rtti::TypeID serializedType = Rtti::TypeID::LoadUnaligned(header.m_typeID);
        if (expectedType.IsValid() && serializedType != expectedType)
        {
            Fail(ErrorCode::kTypeMismatch);
            return false;
        }
        if (header.m_payloadSize > Constants::kMaxU32)
        {
            Fail(ErrorCode::kSizeLimitExceeded);
            return false;
        }

        m_serializedVersion = header.m_version;
        m_serializedSchemaHash = header.m_schemaHash;
        m_impl->m_inputStorage.resize(static_cast<uint32_t>(header.m_payloadSize));
        if (header.m_payloadSize != 0)
        {
            const size_t bytesRead = m_impl->m_stream->ReadToBuffer(m_impl->m_inputStorage.data(), header.m_payloadSize);
            if (bytesRead != header.m_payloadSize)
            {
                Fail(ErrorCode::kStreamReadFailed);
                return false;
            }
        }

        m_impl->m_inputFrames.push_back({ m_impl->m_inputStorage.data(), m_impl->m_inputStorage.size(), 0 });
        return true;
    }


    void TaggedBinaryContext::EndDocument()
    {
        if (!IsSerializing() || !IsValid())
            return;

        FE_Assert(m_impl->m_outputFrames.size() == 1);
        const auto& payload = m_impl->m_outputFrames.back();
        m_impl->m_outputHeader.m_payloadSize = payload.size();
        if (!m_impl->m_stream->Write(m_impl->m_outputHeader))
        {
            Fail(ErrorCode::kStreamWriteFailed);
            return;
        }
        if (!payload.empty() && m_impl->m_stream->WriteFromBuffer(payload.data(), payload.size()) != payload.size())
            Fail(ErrorCode::kStreamWriteFailed);
    }


    bool TaggedBinaryContext::BeginObjectImpl()
    {
        return true;
    }
    void TaggedBinaryContext::EndObjectImpl() {}


    bool TaggedBinaryContext::BeginField(festd::ascii_view, const uint64_t fieldID)
    {
        if (IsSerializing())
        {
            m_impl->m_outputFrames.emplace_back();
            m_impl->m_outputFieldIDs.push_back(fieldID);
            return true;
        }

        const InputFrame& object = m_impl->m_inputFrames.back();
        InputFrame scanner{ object.m_data, object.m_size, 0 };
        while (scanner.m_position < scanner.m_size)
        {
            uint64_t currentID = 0;
            uint64_t fieldSize = 0;
            if (!ReadFrame(scanner, &currentID, sizeof(currentID)))
            {
                Fail(ErrorCode::kMalformedData);
                return false;
            }
            if (!ReadFrame(scanner, &fieldSize, sizeof(fieldSize)))
            {
                Fail(ErrorCode::kMalformedData);
                return false;
            }
            if (fieldSize > scanner.m_size - scanner.m_position)
            {
                Fail(ErrorCode::kMalformedData);
                return false;
            }

            if (currentID == fieldID)
            {
                m_impl->m_inputFrames.push_back({ scanner.m_data + scanner.m_position, static_cast<size_t>(fieldSize), 0 });
                return true;
            }
            scanner.m_position += static_cast<size_t>(fieldSize);
        }

        return false;
    }


    void TaggedBinaryContext::EndField()
    {
        if (IsSerializing())
        {
            auto payload = std::move(m_impl->m_outputFrames.back());
            m_impl->m_outputFrames.pop_back();
            const uint64_t fieldID = m_impl->m_outputFieldIDs.back();
            m_impl->m_outputFieldIDs.pop_back();

            auto& parent = m_impl->m_outputFrames.back();
            Append(parent, fieldID);
            Append(parent, static_cast<uint64_t>(payload.size()));
            AppendBytes(parent, payload.data(), payload.size());
            return;
        }

        m_impl->m_inputFrames.pop_back();
    }


    bool TaggedBinaryContext::BeginArrayImpl(uint32_t& size)
    {
        if (IsSerializing())
        {
            Append(m_impl->m_outputFrames.back(), size);
            return true;
        }

        InputFrame arrayFrame = m_impl->m_inputFrames.back();
        if (!ReadFrame(arrayFrame, &size, sizeof(size)))
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }

        m_impl->m_inputArrays.push_back({ arrayFrame, size });
        return true;
    }


    void TaggedBinaryContext::EndArrayImpl()
    {
        if (IsDeserializing())
            m_impl->m_inputArrays.pop_back();
    }


    bool TaggedBinaryContext::BeginElement(uint32_t)
    {
        if (IsSerializing())
        {
            m_impl->m_outputFrames.emplace_back();
            return true;
        }

        auto& array = m_impl->m_inputArrays.back();
        uint64_t elementSize = 0;
        if (!ReadFrame(array.m_frame, &elementSize, sizeof(elementSize)))
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }
        if (elementSize > array.m_frame.m_size - array.m_frame.m_position)
        {
            Fail(ErrorCode::kMalformedData);
            return false;
        }

        m_impl->m_inputFrames.push_back({ array.m_frame.m_data + array.m_frame.m_position, static_cast<size_t>(elementSize), 0 });
        array.m_frame.m_position += static_cast<size_t>(elementSize);
        return true;
    }


    void TaggedBinaryContext::EndElement()
    {
        if (IsSerializing())
        {
            auto payload = std::move(m_impl->m_outputFrames.back());
            m_impl->m_outputFrames.pop_back();
            auto& parent = m_impl->m_outputFrames.back();
            Append(parent, static_cast<uint64_t>(payload.size()));
            AppendBytes(parent, payload.data(), payload.size());
            return;
        }

        m_impl->m_inputFrames.pop_back();
    }


    void TaggedBinaryContext::StoreScalarImpl(ScalarKind, const void* value, const uint32_t byteSize)
    {
        StoreBytesImpl(value, byteSize);
    }


    void TaggedBinaryContext::LoadScalarImpl(ScalarKind, void* value, const uint32_t byteSize)
    {
        LoadBytesImpl(value, byteSize);
    }


    void TaggedBinaryContext::StoreBytesImpl(const void* value, const uint32_t byteSize)
    {
        AppendBytes(m_impl->m_outputFrames.back(), value, byteSize);
    }


    void TaggedBinaryContext::LoadBytesImpl(void* value, const uint32_t byteSize)
    {
        if (!ReadFrame(m_impl->m_inputFrames.back(), value, byteSize))
            Fail(ErrorCode::kMalformedData);
    }


    void TaggedBinaryContext::StoreStringImpl(const festd::string_view value)
    {
        const uint32_t size = value.size();
        Append(m_impl->m_outputFrames.back(), size);
        AppendBytes(m_impl->m_outputFrames.back(), value.data(), size);
    }


    uint32_t TaggedBinaryContext::LoadStringSizeImpl()
    {
        InputFrame& frame = m_impl->m_inputFrames.back();
        if (!ReadFrame(frame, &m_impl->m_pendingStringSize, sizeof(m_impl->m_pendingStringSize)))
        {
            Fail(ErrorCode::kMalformedData);
            return 0;
        }
        if (m_impl->m_pendingStringSize > frame.m_size - frame.m_position)
        {
            Fail(ErrorCode::kMalformedData);
            return 0;
        }
        return m_impl->m_pendingStringSize;
    }


    void TaggedBinaryContext::LoadStringImpl(const festd::span<char> buffer)
    {
        if (buffer.size() != m_impl->m_pendingStringSize)
        {
            Fail(ErrorCode::kMalformedData);
            return;
        }
        if (!ReadFrame(m_impl->m_inputFrames.back(), buffer.data(), buffer.size()))
        {
            Fail(ErrorCode::kMalformedData);
            return;
        }
        m_impl->m_pendingStringSize = 0;
    }


    uint64_t TaggedBinaryContext::GetCurrentOffset() const
    {
        if (IsSerializing())
            return m_impl->m_stream->Tell();
        if (m_impl->m_inputFrames.empty())
            return m_impl->m_stream->Tell();
        if (m_impl->m_inputStorage.empty())
            return m_impl->m_stream->Tell();

        const InputFrame& frame = m_impl->m_inputFrames.back();
        return sizeof(TaggedHeader) + static_cast<uint64_t>(frame.m_data - m_impl->m_inputStorage.data()) + frame.m_position;
    }
} // namespace FE::Serialization
