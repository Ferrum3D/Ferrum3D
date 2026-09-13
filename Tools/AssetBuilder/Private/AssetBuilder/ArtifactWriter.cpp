#include <AssetBuilder/ArtifactWriter.h>

#include <Core/Serialization/JsonSerialization.h>
#include <Core/Strings/Utils.h>

namespace FE::AssetBuilder
{
    namespace
    {
        IO::Path MakeShardedPath(const IO::Path& root, const festd::string_view category, const Uuid id,
                                 const festd::string_view extension)
        {
            const auto idString = Str::ToLower<festd::fixed_string>(Fmt::FixedFormat("{}", id));
            IO::Path path = root / category;
            path /= festd::string_view{ idString.data(), 2 };
            path /= festd::string_view{ idString.data() + 2, 2 };

            IO::Path filename(idString);
            filename.AsBaseString() += extension;
            return path / filename;
        }


        Uuid MakeUuid(const uint64_t low, const uint64_t high)
        {
            alignas(16) uint64_t words[2] = { low, high };
            Uuid result = Uuid::LoadAligned(words);
            result.m_bytes[6] = (result.m_bytes[6] & 0x0f) | 0x40;
            result.m_bytes[8] = (result.m_bytes[8] & 0x3f) | 0x80;
            return result;
        }
    } // namespace


    ArtifactWriter::ArtifactWriter(const IO::Path& outputRoot, const IO::AssetID assetId, const IO::ArtifactID artifactId,
                                   const Rtti::TypeID assetTypeId)
    {
        const IO::Path absoluteOutputRoot = IO::GetAbsolutePath(outputRoot);
        m_record.m_assetId = assetId;
        m_record.m_artifactId = artifactId;
        m_record.m_assetTypeId = assetTypeId;
        m_dataPath = MakeShardedPath(absoluteOutputRoot, "artifacts/data", artifactId, ".bin");
        m_metadataPath = MakeShardedPath(absoluteOutputRoot, "artifacts/metadata/pc", assetId, ".meta");
    }


    bool ArtifactWriter::OpenDataFile()
    {
        if (m_dataFile)
            return true;

        const IO::ResultCode createResult = IO::Directory::Create(IO::PathView(m_dataPath).parent_directory());
        if (createResult != IO::ResultCode::kSuccess)
        {
            Logger::LogError("Failed to create artifact directory '{}': {}", m_dataPath, IO::GetResultDesc(createResult));
            return false;
        }

        auto fileResult = IO::FileStream::Open(m_dataPath, IO::OpenMode::kCreate);
        if (!fileResult)
        {
            Logger::LogError("Failed to create artifact '{}': {}", m_dataPath, IO::GetResultDesc(fileResult.error()));
            return false;
        }

        m_dataFile = std::move(*fileResult);
        return true;
    }


    bool ArtifactWriter::WritePayload(const festd::span<const std::byte> bytes)
    {
        if (!OpenDataFile())
            return false;

        m_dataFile->FlushWrites();
        IO::ArtifactPayloadRecord& payload = m_record.m_payloads.emplace_back();
        payload.m_resolvedDataSource.m_byteOffset = m_dataFile->Tell();

        auto compressor = Compression::Compressor::Create(Compression::Method::kZstd);
        festd::vector<std::byte> compressedBytes;
        compressedBytes.resize(static_cast<uint32_t>(compressor.GetBounds(Compression::kBlockSize)));

        size_t offset = 0;
        while (offset < bytes.size())
        {
            const size_t uncompressedSize = std::min(bytes.size() - offset, static_cast<size_t>(Compression::kBlockSize));
            const Compression::CompressionResult result =
                compressor.Compress(bytes.data() + offset, uncompressedSize, compressedBytes.data(), compressedBytes.size());
            if (result.m_result != Compression::ResultCode::kSuccess)
            {
                Logger::LogError("Failed to compress artifact payload '{}': chunk at byte {}", m_dataPath, offset);
                return false;
            }

            if (m_dataFile->WriteFromBuffer(compressedBytes.data(), result.m_compressedSize) != result.m_compressedSize)
            {
                Logger::LogError("Failed to write compressed artifact payload '{}': expected {} bytes",
                                 m_dataPath,
                                 result.m_compressedSize);
                return false;
            }

            IO::ArtifactChunkRecord& chunk = payload.m_chunks.emplace_back();
            chunk.m_offsetInPayload = payload.m_resolvedDataSource.m_byteSize;
            chunk.m_compressedSize = result.m_compressedSize;
            chunk.m_uncompressedSize = uncompressedSize;
            chunk.m_compressionMethod = Compression::Method::kZstd;
            chunk.m_checksum.m_current = Crc32::Compute(bytes.data() + offset, uncompressedSize);

            payload.m_resolvedDataSource.m_byteSize += result.m_compressedSize;
            offset += uncompressedSize;
        }

        m_dataFile->FlushWrites();
        return true;
    }


    void ArtifactWriter::AddDependency(const IO::AssetID assetId, const Rtti::TypeID typeId, const IO::DependencyKind kind)
    {
        IO::ArtifactDependencyRecord& dependency = m_record.m_dependencies.emplace_back();
        dependency.m_assetId = assetId;
        dependency.m_expectedTypeId = typeId;
        dependency.m_kind = kind;
    }


    bool ArtifactWriter::WriteMetadata()
    {
        const IO::ResultCode createResult = IO::Directory::Create(IO::PathView(m_metadataPath).parent_directory());
        if (createResult != IO::ResultCode::kSuccess)
            return false;

        auto fileResult = IO::FileStream::Open(m_metadataPath, IO::OpenMode::kCreate);
        if (!fileResult)
            return false;

        Serialization::JsonFormat format;
        Serialization::SerializationContext context(fileResult->Get(), format);
        return context.Store(m_record) == Serialization::ResultCode::kSuccess;
    }


    bool ArtifactWriter::Finish()
    {
        if (!m_dataFile || m_record.m_payloads.empty())
            return false;

        m_dataFile->FlushWrites();
        m_dataFile.Reset();
        if (!WriteMetadata())
        {
            Logger::LogError("Failed to finalize artifact metadata '{}', data remains at '{}'", m_metadataPath, m_dataPath);
            return false;
        }

        Logger::LogInfo("Wrote asset {} as artifact {}", m_record.m_assetId, m_record.m_artifactId);
        return true;
    }


    IO::Path ArtifactWriter::GetDataPath(const IO::Path& outputRoot, const IO::ArtifactID artifactId)
    {
        return MakeShardedPath(IO::GetAbsolutePath(outputRoot), "artifacts/data", artifactId, ".bin");
    }


    bool ArtifactWriter::RemoveArtifact(const IO::Path& outputRoot, const IO::ArtifactID artifactId)
    {
        const IO::Path path = GetDataPath(outputRoot, artifactId);
        const IO::ResultCode result = IO::File::Delete(path);
        if (result == IO::ResultCode::kSuccess)
            return true;

        Logger::LogError("Failed to remove obsolete artifact '{}': {}", path, IO::GetResultDesc(result));
        return false;
    }


    IO::ArtifactID ArtifactWriter::MakeArtifactID(const IO::AssetID assetId, const festd::span<const std::byte> sourceBytes,
                                                  const festd::string_view productKey)
    {
        return MakeArtifactID(assetId, sourceBytes, {}, productKey);
    }


    IO::ArtifactID ArtifactWriter::MakeArtifactID(const IO::AssetID assetId, const festd::span<const std::byte> sourceBytes,
                                                  const festd::span<const std::byte> settingsBytes,
                                                  const festd::string_view productKey)
    {
        Hasher lowHasher(0x04c013886f71ac52ull);
        Hasher highHasher(0xba68ed2194375fc0ull);
        lowHasher.Update(assetId)
            .Update(productKey.data(), productKey.size())
            .Update(settingsBytes.data(), settingsBytes.size())
            .Update(sourceBytes.data(), sourceBytes.size());
        highHasher.Update(sourceBytes.data(), sourceBytes.size())
            .Update(settingsBytes.data(), settingsBytes.size())
            .Update(assetId)
            .Update(productKey.data(), productKey.size());
        return MakeUuid(lowHasher.Finalize(), highHasher.Finalize());
    }
} // namespace FE::AssetBuilder
