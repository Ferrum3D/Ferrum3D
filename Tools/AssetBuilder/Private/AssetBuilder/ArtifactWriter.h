#pragma once
#include <Core/Base/Hash.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/MemoryStream.h>
#include <Core/IO/StreamBase.h>
#include <Core/Serialization/BinarySerialization.h>

namespace FE::AssetBuilder
{
    struct ArtifactWriter final
    {
        ArtifactWriter(const IO::Path& outputRoot, IO::AssetID assetId, Rtti::TypeID assetTypeId);
        ~ArtifactWriter();

        ArtifactWriter(const ArtifactWriter&) = delete;
        ArtifactWriter& operator=(const ArtifactWriter&) = delete;

        template<class T>
        bool WriteHeader(const T& header)
        {
            IO::WriteOnlyMemoryStream stream;
            Serialization::TaggedBinaryFormat format;
            Serialization::SerializationContext context(&stream, format);
            if (context.Store(header) != Serialization::ResultCode::kSuccess)
            {
                Logger::LogError("Failed to serialize asset header");
                return false;
            }

            festd::pmr::vector<std::byte> data;
            stream.DumpAll(data);
            return WritePayload(data);
        }

        bool WriteHeader(const Rtti::Type& type, const void* header);

        bool WritePayload(festd::span<const std::byte> bytes);
        void AddDependency(IO::AssetID assetId, Rtti::TypeID typeId, IO::DependencyKind kind = IO::DependencyKind::kHard);
        bool Finish();

        [[nodiscard]] IO::ArtifactID GetArtifactID() const;

        static IO::Path GetPendingDataPath(const IO::Path& outputRoot, IO::AssetID assetId);
        static IO::Path GetDataPath(const IO::Path& outputRoot, IO::ArtifactID artifactId);
        static IO::Path GetMetadataPath(const IO::Path& outputRoot, IO::AssetID assetId);
        static bool RemoveArtifact(const IO::Path& outputRoot, IO::ArtifactID artifactId);
        static bool RemoveMetadata(const IO::Path& outputRoot, IO::AssetID assetId);

    private:
        bool OpenDataFile();
        bool WriteMetadata();
        void HashCanonicalBytes(const void* data, size_t byteSize);

        IO::Path m_outputRoot;
        IO::Path m_pendingDataPath;
        IO::Path m_dataPath;
        IO::Path m_metadataPath;
        IO::ArtifactRecord m_record;
        Rc<IO::FileStream> m_dataFile;
        Hasher m_lowHasher{ 0x04c013886f71ac52ull };
        Hasher m_highHasher{ 0xba68ed2194375fc0ull };
    };
} // namespace FE::AssetBuilder
