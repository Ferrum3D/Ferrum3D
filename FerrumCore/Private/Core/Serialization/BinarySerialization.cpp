#include <Core/Serialization/BinarySerialization.h>

namespace FE::Serialization
{
    namespace
    {
        constexpr uint32_t kPackedMagic = UINT32_C(0x4b505346);
        constexpr uint32_t kTaggedMagic = UINT32_C(0x47545346);


        struct TaggedHeader final
        {
            uint32_t m_magic;
            uint32_t m_version;
            uint64_t m_schemaHash;
            uint64_t m_payloadSize;
            uint8_t m_typeID[16];
        };


        static_assert(sizeof(TaggedHeader) == 40);
        static_assert(std::is_trivially_copyable_v<TaggedHeader>);


        TaggedHeader CreateTaggedHeader(const Rtti::TypeID type, const uint32_t version, const uint64_t schemaHash)
        {
            TaggedHeader header = {};
            header.m_magic = kTaggedMagic;
            header.m_version = version;
            header.m_schemaHash = schemaHash;
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


    struct PackedBinaryFormat::Impl final
    {
        uint32_t m_pendingStringSize = 0;
    };


    PackedBinaryFormat::PackedBinaryFormat()
        : SerializationFormat(Format::kPackedBinary)
        , m_impl(festd::make_unique<Impl>())
    {
    }


    PackedBinaryFormat::~PackedBinaryFormat() = default;


    void PackedBinaryFormat::ResetImpl()
    {
        m_impl->m_pendingStringSize = 0;
    }


    ResultCode PackedBinaryFormat::BeginStoreDocumentImpl(const Rtti::TypeID expectedType, const uint32_t version,
                                                          const uint64_t schemaHash)
    {
        const uint64_t expectedHeader = HashAll(expectedType, version, schemaHash);
        if (!m_stream->Write(expectedHeader))
            return Fail(ResultCode::kStreamWriteFailed);

        return ResultCode::kSuccess;
    }

    ResultCode PackedBinaryFormat::BeginLoadDocumentImpl(const Rtti::TypeID expectedType, const uint32_t version,
                                                         const uint64_t schemaHash)
    {
        const uint64_t expectedHeader = HashAll(expectedType, version, schemaHash);
        uint64_t header = 0;
        if (!m_stream->Read(header))
            return Fail(ResultCode::kStreamReadFailed);

        if (schemaHash != 0 && header != expectedHeader)
            return Fail(ResultCode::kSchemaMismatch);

        m_serializedVersion = 0;
        m_serializedSchemaHash = header;
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndStoreDocumentImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndLoadDocumentImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginStoreObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginLoadObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndStoreObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndLoadObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginStoreFieldImpl(festd::ascii_view, uint64_t)
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginLoadFieldImpl(festd::ascii_view, uint64_t, bool& exists)
    {
        exists = true;
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndStoreFieldImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndLoadFieldImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginStoreArrayImpl(const uint32_t size)
    {
        if (!m_stream->Write(size))
            return Fail(ResultCode::kStreamWriteFailed);

        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginLoadArrayImpl(uint32_t& size)
    {
        if (!m_stream->Read(size))
            return Fail(ResultCode::kStreamReadFailed);

        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndStoreArrayImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndLoadArrayImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginStoreElementImpl(uint32_t)
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::BeginLoadElementImpl(uint32_t)
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndStoreElementImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::EndLoadElementImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::StoreScalarImpl(ScalarKind, const void* value, const uint32_t byteSize)
    {
        return StoreBytesImpl(value, byteSize);
    }


    ResultCode PackedBinaryFormat::LoadScalarImpl(ScalarKind, void* value, const uint32_t byteSize)
    {
        return LoadBytesImpl(value, byteSize);
    }


    ResultCode PackedBinaryFormat::StoreBytesImpl(const void* value, const uint32_t byteSize)
    {
        if (m_stream->WriteFromBuffer(value, byteSize) != byteSize)
            return Fail(ResultCode::kStreamWriteFailed);

        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::LoadBytesImpl(void* value, const uint32_t byteSize)
    {
        if (m_stream->ReadToBuffer(value, byteSize) != byteSize)
            return Fail(ResultCode::kStreamReadFailed);

        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::StoreStringImpl(const festd::string_view value)
    {
        const uint32_t size = value.size();
        if (!m_stream->Write(size))
            return Fail(ResultCode::kStreamWriteFailed);

        if (size != 0 && m_stream->WriteFromBuffer(value.data(), size) != size)
            return Fail(ResultCode::kStreamWriteFailed);

        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::LoadStringSizeImpl(uint32_t& size)
    {
        if (!m_stream->Read(m_impl->m_pendingStringSize))
            return Fail(ResultCode::kStreamReadFailed);

        size = m_impl->m_pendingStringSize;
        return ResultCode::kSuccess;
    }


    ResultCode PackedBinaryFormat::LoadStringImpl(const festd::span<char> buffer)
    {
        if (buffer.size() != m_impl->m_pendingStringSize)
            return Fail(ResultCode::kMalformedData);

        if (!buffer.empty() && m_stream->ReadToBuffer(buffer.data(), buffer.size()) != buffer.size())
            return Fail(ResultCode::kStreamReadFailed);

        m_impl->m_pendingStringSize = 0;
        return ResultCode::kSuccess;
    }


    uint64_t PackedBinaryFormat::GetStoreCurrentOffsetImpl() const
    {
        return m_stream->Tell();
    }


    uint64_t PackedBinaryFormat::GetLoadCurrentOffsetImpl() const
    {
        return m_stream->Tell();
    }


    struct TaggedBinaryFormat::Impl final
    {
        struct InputArray final
        {
            InputFrame m_frame;
            uint32_t m_count = 0;
        };

        TaggedHeader m_outputHeader{};
        festd::vector<festd::vector<std::byte>> m_outputFrames;
        festd::vector<uint64_t> m_outputFieldIDs;
        festd::vector<InputFrame> m_inputFrames;
        festd::vector<InputArray> m_inputArrays;
        festd::vector<std::byte> m_inputStorage;
        uint32_t m_pendingStringSize = 0;
    };


    TaggedBinaryFormat::TaggedBinaryFormat()
        : SerializationFormat(Format::kTaggedBinary)
        , m_impl(festd::make_unique<Impl>())
    {
    }


    TaggedBinaryFormat::~TaggedBinaryFormat() = default;


    void TaggedBinaryFormat::ResetImpl()
    {
        m_impl->m_outputHeader = {};
        m_impl->m_outputFrames.clear();
        m_impl->m_outputFieldIDs.clear();
        m_impl->m_inputFrames.clear();
        m_impl->m_inputArrays.clear();
        m_impl->m_inputStorage.clear();
        m_impl->m_pendingStringSize = 0;
    }


    ResultCode TaggedBinaryFormat::BeginStoreDocumentImpl(const Rtti::TypeID expectedType, const uint32_t version,
                                                          const uint64_t schemaHash)
    {
        m_impl->m_outputHeader = CreateTaggedHeader(expectedType, version, schemaHash);
        m_impl->m_outputFrames.emplace_back();
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::BeginLoadDocumentImpl(const Rtti::TypeID expectedType, uint32_t, uint64_t)
    {
        TaggedHeader header{};
        if (!m_stream->Read(header))
            return Fail(ResultCode::kStreamReadFailed);

        if (header.m_magic != kTaggedMagic)
            return Fail(ResultCode::kInvalidHeader);

        const Rtti::TypeID serializedType = Rtti::TypeID::LoadUnaligned(header.m_typeID);
        if (expectedType.IsValid() && serializedType != expectedType)
            return Fail(ResultCode::kTypeMismatch);

        FE_Assert(header.m_payloadSize <= Constants::kMaxU32);

        m_serializedVersion = header.m_version;
        m_serializedSchemaHash = header.m_schemaHash;
        m_impl->m_inputStorage.resize(static_cast<uint32_t>(header.m_payloadSize));
        if (header.m_payloadSize != 0)
        {
            const size_t bytesRead = m_stream->ReadToBuffer(m_impl->m_inputStorage.data(), header.m_payloadSize);
            if (bytesRead != header.m_payloadSize)
                return Fail(ResultCode::kStreamReadFailed);
        }

        m_impl->m_inputFrames.push_back({ m_impl->m_inputStorage.data(), m_impl->m_inputStorage.size(), 0 });
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndStoreDocumentImpl()
    {
        if (!IsValid())
            return m_error.m_code;

        FE_Assert(m_impl->m_outputFrames.size() == 1);
        const auto& payload = m_impl->m_outputFrames.back();
        m_impl->m_outputHeader.m_payloadSize = payload.size();
        if (!m_stream->Write(m_impl->m_outputHeader))
            return Fail(ResultCode::kStreamWriteFailed);

        if (!payload.empty() && m_stream->WriteFromBuffer(payload.data(), payload.size()) != payload.size())
            return Fail(ResultCode::kStreamWriteFailed);

        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndLoadDocumentImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::BeginStoreObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::BeginLoadObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndStoreObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndLoadObjectImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::BeginStoreFieldImpl(festd::ascii_view, const uint64_t fieldID)
    {
        m_impl->m_outputFrames.emplace_back();
        m_impl->m_outputFieldIDs.push_back(fieldID);
        return ResultCode::kSuccess;
    }

    ResultCode TaggedBinaryFormat::BeginLoadFieldImpl(festd::ascii_view, const uint64_t fieldID, bool& exists)
    {
        exists = true;

        const InputFrame& object = m_impl->m_inputFrames.back();
        InputFrame scanner{ object.m_data, object.m_size, 0 };
        while (scanner.m_position < scanner.m_size)
        {
            uint64_t currentID = 0;
            uint64_t fieldSize = 0;
            if (!ReadFrame(scanner, &currentID, sizeof(currentID)))
                return Fail(ResultCode::kMalformedData);

            if (!ReadFrame(scanner, &fieldSize, sizeof(fieldSize)))
                return Fail(ResultCode::kMalformedData);

            if (fieldSize > scanner.m_size - scanner.m_position)
                return Fail(ResultCode::kMalformedData);

            if (currentID == fieldID)
            {
                m_impl->m_inputFrames.push_back({ scanner.m_data + scanner.m_position, static_cast<size_t>(fieldSize), 0 });
                return ResultCode::kSuccess;
            }

            scanner.m_position += fieldSize;
        }

        exists = false;
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndStoreFieldImpl()
    {
        auto payload = std::move(m_impl->m_outputFrames.back());
        m_impl->m_outputFrames.pop_back();
        const uint64_t fieldID = m_impl->m_outputFieldIDs.back();
        m_impl->m_outputFieldIDs.pop_back();

        auto& parent = m_impl->m_outputFrames.back();
        Append(parent, fieldID);
        Append(parent, static_cast<uint64_t>(payload.size()));
        AppendBytes(parent, payload.data(), payload.size());
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndLoadFieldImpl()
    {
        m_impl->m_inputFrames.pop_back();
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::BeginStoreArrayImpl(const uint32_t size)
    {
        Append(m_impl->m_outputFrames.back(), size);
        return ResultCode::kSuccess;
    }

    ResultCode TaggedBinaryFormat::BeginLoadArrayImpl(uint32_t& size)
    {
        InputFrame arrayFrame = m_impl->m_inputFrames.back();
        if (!ReadFrame(arrayFrame, &size, sizeof(size)))
            return Fail(ResultCode::kMalformedData);

        m_impl->m_inputArrays.push_back({ arrayFrame, size });
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndStoreArrayImpl()
    {
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndLoadArrayImpl()
    {
        m_impl->m_inputArrays.pop_back();
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::BeginStoreElementImpl(uint32_t)
    {
        m_impl->m_outputFrames.emplace_back();
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::BeginLoadElementImpl(uint32_t)
    {
        auto& array = m_impl->m_inputArrays.back();
        uint64_t elementSize = 0;
        if (!ReadFrame(array.m_frame, &elementSize, sizeof(elementSize)))
            return Fail(ResultCode::kMalformedData);

        if (elementSize > array.m_frame.m_size - array.m_frame.m_position)
            return Fail(ResultCode::kMalformedData);

        m_impl->m_inputFrames.push_back({ array.m_frame.m_data + array.m_frame.m_position, static_cast<size_t>(elementSize), 0 });
        array.m_frame.m_position += static_cast<size_t>(elementSize);
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndStoreElementImpl()
    {
        auto payload = std::move(m_impl->m_outputFrames.back());
        m_impl->m_outputFrames.pop_back();
        auto& parent = m_impl->m_outputFrames.back();
        Append(parent, static_cast<uint64_t>(payload.size()));
        AppendBytes(parent, payload.data(), payload.size());
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::EndLoadElementImpl()
    {
        m_impl->m_inputFrames.pop_back();
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::StoreScalarImpl(ScalarKind, const void* value, const uint32_t byteSize)
    {
        return StoreBytesImpl(value, byteSize);
    }


    ResultCode TaggedBinaryFormat::LoadScalarImpl(ScalarKind, void* value, const uint32_t byteSize)
    {
        return LoadBytesImpl(value, byteSize);
    }


    ResultCode TaggedBinaryFormat::StoreBytesImpl(const void* value, const uint32_t byteSize)
    {
        AppendBytes(m_impl->m_outputFrames.back(), value, byteSize);
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::LoadBytesImpl(void* value, const uint32_t byteSize)
    {
        if (!ReadFrame(m_impl->m_inputFrames.back(), value, byteSize))
            return Fail(ResultCode::kMalformedData);

        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::StoreStringImpl(const festd::string_view value)
    {
        const uint32_t size = value.size();
        Append(m_impl->m_outputFrames.back(), size);
        AppendBytes(m_impl->m_outputFrames.back(), value.data(), size);
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::LoadStringSizeImpl(uint32_t& size)
    {
        InputFrame& frame = m_impl->m_inputFrames.back();
        if (!ReadFrame(frame, &m_impl->m_pendingStringSize, sizeof(m_impl->m_pendingStringSize)))
            return Fail(ResultCode::kMalformedData);

        if (m_impl->m_pendingStringSize > frame.m_size - frame.m_position)
            return Fail(ResultCode::kMalformedData);

        size = m_impl->m_pendingStringSize;
        return ResultCode::kSuccess;
    }


    ResultCode TaggedBinaryFormat::LoadStringImpl(const festd::span<char> buffer)
    {
        if (buffer.size() != m_impl->m_pendingStringSize)
            return Fail(ResultCode::kMalformedData);

        if (!ReadFrame(m_impl->m_inputFrames.back(), buffer.data(), buffer.size()))
            return Fail(ResultCode::kMalformedData);

        m_impl->m_pendingStringSize = 0;
        return ResultCode::kSuccess;
    }


    uint64_t TaggedBinaryFormat::GetStoreCurrentOffsetImpl() const
    {
        return m_stream->Tell();
    }


    uint64_t TaggedBinaryFormat::GetLoadCurrentOffsetImpl() const
    {
        if (m_impl->m_inputFrames.empty())
            return m_stream->Tell();

        if (m_impl->m_inputStorage.empty())
            return m_stream->Tell();

        const InputFrame& frame = m_impl->m_inputFrames.back();
        return sizeof(TaggedHeader) + static_cast<uint64_t>(frame.m_data - m_impl->m_inputStorage.data()) + frame.m_position;
    }
} // namespace FE::Serialization
