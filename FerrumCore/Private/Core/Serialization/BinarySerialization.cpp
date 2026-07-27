#include <Core/Serialization/BinarySerialization.h>

namespace FE::Serialization
{
    namespace
    {
        inline constexpr uint32_t kPackedMagic = UINT32_C(0x4b505346);
        inline constexpr uint32_t kTaggedMagic = UINT32_C(0x47545346);


        template<class T>
        bool Write(IO::IStream* stream, const T& value)
        {
            return stream->WriteFromBuffer(&value, sizeof(value)) == sizeof(value);
        }


        template<class T>
        bool Read(IO::IStream* stream, T& value)
        {
            return stream->ReadToBuffer(&value, sizeof(value)) == sizeof(value);
        }


        bool WriteHeader(IO::IStream* stream, const uint32_t magic, const Rtti::TypeID type, const uint32_t version,
                         const uint64_t schemaHash)
        {
            return Write(stream, magic) && stream->WriteFromBuffer(type.data(), type.size()) == type.size()
                && Write(stream, version) && Write(stream, schemaHash);
        }


        bool ReadHeader(IO::IStream* stream, const uint32_t expectedMagic, const Rtti::TypeID expectedType,
                        const uint64_t expectedSchema, uint32_t& version, uint64_t& schemaHash)
        {
            uint32_t magic = 0;
            Rtti::TypeID type;
            if (!Read(stream, magic) || magic != expectedMagic || stream->ReadToBuffer(type.data(), type.size()) != type.size()
                || !Read(stream, version) || !Read(stream, schemaHash))
            {
                return false;
            }

            if (expectedType.IsValid() && type != expectedType)
                return false;

            return expectedSchema == 0 || schemaHash == expectedSchema;
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
            if (size > frame.m_size - frame.m_position)
                return false;

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
    };


    PackedBinaryContext::PackedBinaryContext(IO::IStream* stream)
        : SerializationContext(Format::kPackedBinary)
        , m_impl(std::make_unique<Impl>(stream))
    {
        FE_Assert(stream != nullptr);
    }


    PackedBinaryContext::~PackedBinaryContext() = default;


    void PackedBinaryContext::Reset() {}


    bool PackedBinaryContext::BeginDocument(const Rtti::TypeID expectedType, const uint32_t version, const uint64_t schemaHash)
    {
        if (IsSerializing())
            return WriteHeader(m_impl->m_stream, kPackedMagic, expectedType, version, schemaHash);

        const bool result =
            ReadHeader(m_impl->m_stream, kPackedMagic, expectedType, schemaHash, m_serializedVersion, m_serializedSchemaHash);
        if (!result)
            Fail();
        return result;
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
        if (IsSerializing())
        {
            if (!Write(m_impl->m_stream, size))
                Fail();
        }
        else if (!Read(m_impl->m_stream, size))
        {
            Fail();
        }
        return IsValid();
    }


    void PackedBinaryContext::EndArrayImpl() {}
    bool PackedBinaryContext::BeginElement(uint32_t)
    {
        return true;
    }
    void PackedBinaryContext::EndElement() {}


    void PackedBinaryContext::TransferScalar(ScalarKind, void* value, const uint32_t byteSize)
    {
        TransferBytes(value, byteSize);
    }


    void PackedBinaryContext::TransferBytes(void* value, const uint32_t byteSize)
    {
        const size_t transferred = IsSerializing() ? m_impl->m_stream->WriteFromBuffer(value, byteSize)
                                                   : m_impl->m_stream->ReadToBuffer(value, byteSize);
        if (transferred != byteSize)
            Fail();
    }


    void PackedBinaryContext::TransferString(festd::string* output, const festd::string_view input)
    {
        uint32_t size = output ? 0 : input.size();
        if (IsSerializing())
        {
            if (!Write(m_impl->m_stream, size) || (size != 0 && m_impl->m_stream->WriteFromBuffer(input.data(), size) != size))
                Fail();
            return;
        }

        if (!Read(m_impl->m_stream, size))
        {
            Fail();
            return;
        }

        output->resize_uninitialized(size);
        if (size != 0 && m_impl->m_stream->ReadToBuffer(output->data(), size) != size)
            Fail();
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
        festd::vector<festd::vector<std::byte>> m_outputFrames;
        festd::vector<uint64_t> m_outputFieldIDs;
        festd::vector<InputFrame> m_inputFrames;
        festd::vector<InputArray> m_inputArrays;
        festd::vector<std::byte> m_inputStorage;
    };


    TaggedBinaryContext::TaggedBinaryContext(IO::IStream* stream)
        : SerializationContext(Format::kTaggedBinary)
        , m_impl(std::make_unique<Impl>(stream))
    {
        FE_Assert(stream != nullptr);
    }


    TaggedBinaryContext::~TaggedBinaryContext() = default;


    void TaggedBinaryContext::Reset()
    {
        m_impl->m_outputFrames.clear();
        m_impl->m_outputFieldIDs.clear();
        m_impl->m_inputFrames.clear();
        m_impl->m_inputArrays.clear();
        m_impl->m_inputStorage.clear();
    }


    bool TaggedBinaryContext::BeginDocument(const Rtti::TypeID expectedType, const uint32_t version, const uint64_t schemaHash)
    {
        if (IsSerializing())
        {
            if (!WriteHeader(m_impl->m_stream, kTaggedMagic, expectedType, version, schemaHash))
            {
                Fail();
                return false;
            }
            m_impl->m_outputFrames.emplace_back();
            return true;
        }

        uint32_t magic = 0;
        Rtti::TypeID type;
        uint64_t payloadSize = 0;
        if (!Read(m_impl->m_stream, magic) || magic != kTaggedMagic
            || m_impl->m_stream->ReadToBuffer(type.data(), type.size()) != type.size()
            || !Read(m_impl->m_stream, m_serializedVersion) || !Read(m_impl->m_stream, m_serializedSchemaHash)
            || !Read(m_impl->m_stream, payloadSize) || (expectedType.IsValid() && type != expectedType)
            || payloadSize > Constants::kMaxU32)
        {
            Fail();
            return false;
        }

        m_impl->m_inputStorage.resize(static_cast<uint32_t>(payloadSize));
        if (payloadSize != 0 && m_impl->m_stream->ReadToBuffer(m_impl->m_inputStorage.data(), payloadSize) != payloadSize)
        {
            Fail();
            return false;
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
        const uint64_t payloadSize = payload.size();
        if (!Write(m_impl->m_stream, payloadSize)
            || (payloadSize != 0 && m_impl->m_stream->WriteFromBuffer(payload.data(), payloadSize) != payloadSize))
            Fail();
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
            if (!ReadFrame(scanner, &currentID, sizeof(currentID)) || !ReadFrame(scanner, &fieldSize, sizeof(fieldSize))
                || fieldSize > scanner.m_size - scanner.m_position)
            {
                Fail();
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
        }
        else
        {
            m_impl->m_inputFrames.pop_back();
        }
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
            Fail();
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
        if (!ReadFrame(array.m_frame, &elementSize, sizeof(elementSize))
            || elementSize > array.m_frame.m_size - array.m_frame.m_position)
        {
            Fail();
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
        }
        else
        {
            m_impl->m_inputFrames.pop_back();
        }
    }


    void TaggedBinaryContext::TransferScalar(ScalarKind, void* value, const uint32_t byteSize)
    {
        TransferBytes(value, byteSize);
    }


    void TaggedBinaryContext::TransferBytes(void* value, const uint32_t byteSize)
    {
        if (IsSerializing())
        {
            AppendBytes(m_impl->m_outputFrames.back(), value, byteSize);
        }
        else if (!ReadFrame(m_impl->m_inputFrames.back(), value, byteSize))
        {
            Fail();
        }
    }


    void TaggedBinaryContext::TransferString(festd::string* output, const festd::string_view input)
    {
        uint32_t size = output ? 0 : input.size();
        if (IsSerializing())
        {
            Append(m_impl->m_outputFrames.back(), size);
            AppendBytes(m_impl->m_outputFrames.back(), input.data(), size);
            return;
        }

        InputFrame& frame = m_impl->m_inputFrames.back();
        if (!ReadFrame(frame, &size, sizeof(size)) || size > frame.m_size - frame.m_position)
        {
            Fail();
            return;
        }

        output->assign(reinterpret_cast<const char*>(frame.m_data + frame.m_position), size);
        frame.m_position += size;
    }
} // namespace FE::Serialization
