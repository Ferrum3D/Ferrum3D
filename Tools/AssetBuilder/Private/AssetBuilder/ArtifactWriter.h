#pragma once
#include <Core/IO/Artifact.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/StreamBase.h>
#include <Core/Serialization/BinarySerialization.h>

namespace FE::AssetBuilder
{
    namespace Internal
    {
        struct ArtifactMemoryOutputStream final : public IO::BufferedStream
        {
            ArtifactMemoryOutputStream()
                : BufferedStream(nullptr)
            {
            }

            ~ArtifactMemoryOutputStream() override
            {
                FlushWrites();
            }

            [[nodiscard]] bool SeekAllowed() const override
            {
                return false;
            }

            [[nodiscard]] bool IsOpen() const override
            {
                return true;
            }

            IO::ResultCode Seek(intptr_t, IO::SeekMode) override
            {
                return IO::ResultCode::kInvalidSeek;
            }

            [[nodiscard]] uintptr_t Tell() const override
            {
                return m_data.size() + m_bufferPosition;
            }

            [[nodiscard]] size_t Length() const override
            {
                return m_data.size() + m_bufferPosition;
            }

            size_t ReadToBuffer(void*, size_t) override
            {
                return 0;
            }

            [[nodiscard]] festd::string_view GetName() override
            {
                return "ArtifactHeader";
            }

            [[nodiscard]] IO::OpenMode GetOpenMode() const override
            {
                return IO::OpenMode::kWriteOnly;
            }

            void Close() override {}

            [[nodiscard]] festd::span<const std::byte> GetData()
            {
                FlushWrites();
                return m_data;
            }

        private:
            festd::vector<std::byte> m_data;

            void DoRelease() override
            {
                FE_Assert(false, "Stack-owned stream cannot be released");
            }

            size_t WriteImpl(const void* buffer, const size_t byteSize) override
            {
                const uint32_t offset = m_data.size();
                m_data.resize(offset + static_cast<uint32_t>(byteSize));
                memcpy(m_data.data() + offset, buffer, byteSize);
                return byteSize;
            }
        };
    } // namespace Internal


    struct ArtifactWriter final
    {
        ArtifactWriter(const IO::Path& outputRoot, IO::AssetID assetId, IO::ArtifactID artifactId, Rtti::TypeID assetTypeId);

        ArtifactWriter(const ArtifactWriter&) = delete;
        ArtifactWriter& operator=(const ArtifactWriter&) = delete;

        template<class T>
        bool WriteHeader(const T& header)
        {
            Internal::ArtifactMemoryOutputStream stream;
            Serialization::TaggedBinaryFormat format;
            Serialization::SerializationContext context(&stream, format);
            if (context.Store(header) != Serialization::ResultCode::kSuccess)
            {
                Logger::LogError("Failed to serialize asset header");
                return false;
            }

            return WritePayload(stream.GetData());
        }

        bool WritePayload(festd::span<const std::byte> bytes);
        void AddDependency(IO::AssetID assetId, Rtti::TypeID typeId, IO::DependencyKind kind = IO::DependencyKind::kHard);
        bool Finish();

        static IO::ArtifactID MakeArtifactID(IO::AssetID assetId, festd::span<const std::byte> sourceBytes,
                                             festd::string_view productKey);
        static IO::ArtifactID MakeArtifactID(IO::AssetID assetId, festd::span<const std::byte> sourceBytes,
                                             festd::span<const std::byte> settingsBytes, festd::string_view productKey);
        static IO::Path GetDataPath(const IO::Path& outputRoot, IO::ArtifactID artifactId);
        static bool RemoveArtifact(const IO::Path& outputRoot, IO::ArtifactID artifactId);

    private:
        bool OpenDataFile();
        bool WriteMetadata();

        IO::Path m_dataPath;
        IO::Path m_metadataPath;
        IO::ArtifactRecord m_record;
        Rc<IO::FileStream> m_dataFile;
    };
} // namespace FE::AssetBuilder
