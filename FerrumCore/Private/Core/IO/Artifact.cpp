#include <Core/IO/Artifact.h>
#include <Core/IO/StreamBase.h>
#include <Core/RTTI/Reflection.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Core/Serialization/Serialization.h>

namespace FE::IO
{
    namespace
    {
        constexpr uint32_t kMaxDependencies = 4096;
        constexpr uint32_t kMaxPayloads = 256;
        constexpr uint32_t kMaxChunksPerPayload = 65536;
        constexpr uint64_t kMaxPayloadSize = Constants::kMaxU32;
        constexpr size_t kMaxMetadataSize = 1024 * 1024;

        struct MetadataDependency final
        {
            AssetID m_asset = AssetID::kNull;
            Rtti::TypeID m_type = Rtti::TypeID::kNull;
            festd::basic_inline_string<16> m_kind;

            static Serialization::ResultCode Serialize(Serialization::SerializationContext& context,
                                                       const MetadataDependency& value)
            {
                if (auto object = context.BeginObject())
                    object.Field("asset", value.m_asset).Field("type", value.m_type).Field("kind", value.m_kind);
                return context.GetResultCode();
            }

            static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context,
                                                         MetadataDependency& value)
            {
                if (auto object = context.BeginObject())
                    object.RequiredField("asset", value.m_asset)
                        .RequiredField("type", value.m_type)
                        .RequiredField("kind", value.m_kind);
                return context.GetResultCode();
            }
        };

        struct MetadataChunk final
        {
            uint64_t m_offset = 0;
            uint64_t m_compressedSize = 0;
            uint64_t m_uncompressedSize = 0;
            festd::basic_inline_string<16> m_compression;
            uint32_t m_checksum = 0;

            static Serialization::ResultCode Serialize(Serialization::SerializationContext& context, const MetadataChunk& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.Field("offset", value.m_offset)
                        .Field("compressedSize", value.m_compressedSize)
                        .Field("uncompressedSize", value.m_uncompressedSize)
                        .Field("compression", value.m_compression)
                        .Field("checksum", value.m_checksum);
                }
                return context.GetResultCode();
            }

            static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, MetadataChunk& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.RequiredField("offset", value.m_offset)
                        .RequiredField("compressedSize", value.m_compressedSize)
                        .RequiredField("uncompressedSize", value.m_uncompressedSize)
                        .RequiredField("compression", value.m_compression)
                        .RequiredField("checksum", value.m_checksum);
                }
                return context.GetResultCode();
            }
        };

        struct MetadataPayload final
        {
            Path m_source;
            uint64_t m_offset = 0;
            uint64_t m_size = 0;
            festd::vector<MetadataChunk> m_chunks;

            static Serialization::ResultCode Serialize(Serialization::SerializationContext& context, const MetadataPayload& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.Field("source", value.m_source)
                        .Field("offset", value.m_offset)
                        .Field("size", value.m_size)
                        .Field("chunks", value.m_chunks);
                }
                return context.GetResultCode();
            }

            static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, MetadataPayload& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.Field("source", value.m_source)
                        .RequiredField("offset", value.m_offset)
                        .RequiredField("size", value.m_size)
                        .RequiredField("chunks", value.m_chunks);
                }
                return context.GetResultCode();
            }
        };

        struct MetadataDocument final
        {
            static constexpr uint32_t kVersion = ArtifactStore::kMetadataVersion;
            static constexpr uint64_t RTTI_GetSerializationSchemaHash()
            {
                return 0x6172746966616374ull;
            }

            festd::basic_inline_string<24> m_schema;
            festd::basic_inline_string<32> m_platform;
            AssetID m_asset = AssetID::kNull;
            ArtifactID m_artifact = ArtifactID::kNull;
            Rtti::TypeID m_type = Rtti::TypeID::kNull;
            festd::vector<MetadataDependency> m_dependencies;
            festd::vector<MetadataPayload> m_payloads;

            static Serialization::ResultCode Serialize(Serialization::SerializationContext& context,
                                                       const MetadataDocument& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.Field("schema", value.m_schema)
                        .Field("platform", value.m_platform)
                        .Field("asset", value.m_asset)
                        .Field("artifact", value.m_artifact)
                        .Field("type", value.m_type)
                        .Field("dependencies", value.m_dependencies)
                        .Field("payloads", value.m_payloads);
                }
                return context.GetResultCode();
            }

            static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, MetadataDocument& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.RequiredField("schema", value.m_schema)
                        .RequiredField("platform", value.m_platform)
                        .RequiredField("asset", value.m_asset)
                        .RequiredField("artifact", value.m_artifact)
                        .RequiredField("type", value.m_type)
                        .RequiredField("dependencies", value.m_dependencies)
                        .RequiredField("payloads", value.m_payloads);
                }
                return context.GetResultCode();
            }
        };

        ArtifactDecodeResult Fail(const ArtifactResolutionContext& context, const ArtifactMetadataErrorCode code,
                                  const festd::string_view field, const festd::string_view message)
        {
            ArtifactMetadataError error;
            error.m_code = code;
            error.m_assetId = context.m_assetId;
            error.m_source = context.m_metadataSource.m_filePath;
            error.m_field.assign(field.data(), field.size());
            error.m_message.assign(message.data(), message.size());
            return festd::unexpected(std::move(error));
        }

        bool ParseCompression(const festd::string_view name, Compression::Method& result)
        {
            if (name == "none")
                result = Compression::Method::kNone;
            else if (name == "deflate")
                result = Compression::Method::kDeflate;
            else if (name == "zstd")
                result = Compression::Method::kZstd;
            else
                return false;
            return true;
        }

        bool IsSafeRelativeSource(const Path& source)
        {
            if (source.empty())
                return false;
            const festd::string_view sourceView = source;
            if (!PathView(sourceView).is_relative())
                return false;
            bool isSafe = true;
            TraversePath(sourceView, [&isSafe](const festd::string_view component) {
                if (component == "..")
                    isSafe = false;
            });
            for (uint32_t index = 0; index < source.size(); ++index)
            {
                if (source.data()[index] == ':')
                    isSafe = false;
            }
            return isSafe;
        }

        ArtifactMetadataErrorCode MapSerializationError(const Serialization::ResultCode result)
        {
            if (result == Serialization::ResultCode::kMissingField)
                return ArtifactMetadataErrorCode::kMissingField;
            if (result == Serialization::ResultCode::kTypeMismatch)
                return ArtifactMetadataErrorCode::kUnsupportedSchema;
            return ArtifactMetadataErrorCode::kInvalidJson;
        }
    } // namespace

    struct ArtifactStore::Impl final
    {
        Path m_assetDirectoryPath;
        festd::basic_inline_string<32> m_platform = "windows-x64";
    };

    ArtifactStore::Impl* ArtifactStore::GImpl = nullptr;

    void ArtifactStore::Init()
    {
        FE_Assert(GImpl == nullptr, "Artifact Store already initialized");
        GImpl = Memory::DefaultNew<Impl>();
    }

    void ArtifactStore::Shutdown()
    {
        FE_Assert(GImpl != nullptr, "Artifact Store not initialized");
        Memory::DefaultDelete(GImpl);
        GImpl = nullptr;
    }

    ResolvedDataSource ArtifactStore::ResolveMeta(const AssetID assetID)
    {
        const auto idString = Str::ToLower<festd::fixed_string>(Fmt::FixedFormat("{}", assetID));
        Path relativePath("artifacts");
        relativePath /= festd::string_view{ idString.data(), 2 };
        relativePath /= festd::string_view{ idString.data() + 2, 2 };
        Path fileName(idString);
        fileName.AsBaseString().append(".meta.json");
        relativePath /= fileName;
        return ResolvedDataSource{ GImpl->m_assetDirectoryPath / relativePath };
    }

    ResolvedDataSource ArtifactStore::ResolveData(const ArtifactID artifactID)
    {
        const auto idString = Str::ToLower<festd::fixed_string>(Fmt::FixedFormat("{}", artifactID));
        Path relativePath("artifacts");
        relativePath /= festd::string_view{ idString.data(), 2 };
        relativePath /= festd::string_view{ idString.data() + 2, 2 };
        Path fileName(idString);
        fileName.AsBaseString().append(".data");
        relativePath /= fileName;
        return ResolvedDataSource{ GImpl->m_assetDirectoryPath / relativePath };
    }

    ArtifactDecodeResult ArtifactStore::Decode(const festd::span<const std::byte> bytes, const ArtifactResolutionContext& context)
    {
        if (bytes.size() > kMaxMetadataSize)
            return Fail(context, ArtifactMetadataErrorCode::kLimitExceeded, "$", "metadata exceeds the one MiB limit");

        ReadOnlyMemoryStream stream(bytes);
        Serialization::JsonFormat format;
        Serialization::DeserializationContext deserializationContext(&stream, format);
        MetadataDocument metadata;
        const Serialization::ResultCode deserializeResult = deserializationContext.Load(metadata);
        if (deserializeResult != Serialization::ResultCode::kSuccess)
        {
            const ArtifactMetadataErrorCode code = MapSerializationError(deserializeResult);
            return Fail(context, code, "$", "metadata does not match the JSON artifact schema");
        }
        if (deserializationContext.GetSerializedVersion() != kMetadataVersion)
            return Fail(context, ArtifactMetadataErrorCode::kUnsupportedSchema, "$version", "unsupported metadata version");
        if (deserializationContext.GetSerializedSchemaHash() != MetadataDocument::RTTI_GetSerializationSchemaHash())
            return Fail(context, ArtifactMetadataErrorCode::kUnsupportedSchema, "$schema", "unsupported metadata schema hash");
        if (metadata.m_schema != "ferrum-artifact")
            return Fail(context, ArtifactMetadataErrorCode::kUnsupportedSchema, "schema", "unsupported metadata schema");
        if (metadata.m_platform != GImpl->m_platform)
            return Fail(context,
                        ArtifactMetadataErrorCode::kPlatformMismatch,
                        "platform",
                        "metadata targets a different platform");
        if (metadata.m_asset != context.m_assetId)
            return Fail(context,
                        ArtifactMetadataErrorCode::kIdentityMismatch,
                        "asset",
                        "asset does not match the requested asset");
        const bool artifactMatches = !context.m_artifactId.IsValid() || metadata.m_artifact == context.m_artifactId;
        if (!artifactMatches)
            return Fail(context,
                        ArtifactMetadataErrorCode::kIdentityMismatch,
                        "artifact",
                        "artifact does not match the requested artifact");
        if (Rtti::TypeRegistry::FindType(metadata.m_type) == nullptr)
            return Fail(context, ArtifactMetadataErrorCode::kUnknownType, "type", "asset type ID is not registered");
        if (metadata.m_dependencies.size() > kMaxDependencies)
            return Fail(context,
                        ArtifactMetadataErrorCode::kLimitExceeded,
                        "dependencies",
                        "dependency count exceeds the metadata limit");
        if (metadata.m_payloads.empty())
            return Fail(context, ArtifactMetadataErrorCode::kInvalidLayout, "payloads", "payload zero is required");
        if (metadata.m_payloads.size() > kMaxPayloads)
            return Fail(context,
                        ArtifactMetadataErrorCode::kLimitExceeded,
                        "payloads",
                        "payload count exceeds the metadata limit");

        ArtifactRecord result;
        result.m_assetId = metadata.m_asset;
        result.m_artifactId = metadata.m_artifact;
        result.m_assetTypeId = metadata.m_type;
        for (const MetadataDependency& dependency : metadata.m_dependencies)
        {
            if (!dependency.m_asset.IsValid())
                return Fail(context,
                            ArtifactMetadataErrorCode::kInvalidField,
                            "dependencies.asset",
                            "dependency asset must be nonzero");
            if (Rtti::TypeRegistry::FindType(dependency.m_type) == nullptr)
                return Fail(context,
                            ArtifactMetadataErrorCode::kUnknownType,
                            "dependencies.type",
                            "dependency type ID is not registered");
            ArtifactDependencyRecord record;
            record.m_assetId = dependency.m_asset;
            record.m_expectedTypeId = dependency.m_type;
            if (dependency.m_kind == "hard")
                record.m_kind = DependencyKind::kHard;
            else if (dependency.m_kind == "soft")
                record.m_kind = DependencyKind::kSoft;
            else
                return Fail(context,
                            ArtifactMetadataErrorCode::kInvalidField,
                            "dependencies.kind",
                            "dependency kind must be hard or soft");
            result.m_dependencies.push_back(record);
        }

        for (const MetadataPayload& metadataPayload : metadata.m_payloads)
        {
            const bool isValidPayloadSize = metadataPayload.m_size > 0 && metadataPayload.m_size <= kMaxPayloadSize;
            if (!isValidPayloadSize)
                return Fail(context,
                            ArtifactMetadataErrorCode::kLimitExceeded,
                            "payloads.size",
                            "payload size is zero or exceeds the limit");
            if (metadataPayload.m_offset > Constants::kMaxValue<size_t> - metadataPayload.m_size)
                return Fail(context, ArtifactMetadataErrorCode::kInvalidLayout, "payloads", "payload range overflows");
            if (metadataPayload.m_chunks.empty())
                return Fail(context,
                            ArtifactMetadataErrorCode::kInvalidLayout,
                            "payloads.chunks",
                            "payload must contain at least one chunk");
            if (metadataPayload.m_chunks.size() > kMaxChunksPerPayload)
                return Fail(context,
                            ArtifactMetadataErrorCode::kLimitExceeded,
                            "payloads.chunks",
                            "chunk count exceeds the metadata limit");

            ArtifactPayloadRecord payload;
            if (metadataPayload.m_source.empty())
                payload.m_resolvedDataSource = ResolveData(metadata.m_artifact);
            else
            {
                if (!IsSafeRelativeSource(metadataPayload.m_source))
                    return Fail(context,
                                ArtifactMetadataErrorCode::kInvalidField,
                                "payloads.source",
                                "payload source must be store-relative");
                payload.m_resolvedDataSource.m_filePath = GImpl->m_assetDirectoryPath / metadataPayload.m_source;
            }
            payload.m_resolvedDataSource.m_byteOffset = static_cast<size_t>(metadataPayload.m_offset);
            payload.m_resolvedDataSource.m_byteSize = static_cast<size_t>(metadataPayload.m_size);

            uint64_t expectedOffset = 0;
            uint64_t outputSize = 0;
            for (const MetadataChunk& metadataChunk : metadataPayload.m_chunks)
            {
                ArtifactChunkRecord chunk;
                chunk.m_offsetInPayload = metadataChunk.m_offset;
                chunk.m_compressedSize = metadataChunk.m_compressedSize;
                chunk.m_uncompressedSize = metadataChunk.m_uncompressedSize;
                chunk.m_checksum.m_current = metadataChunk.m_checksum;
                if (!ParseCompression(metadataChunk.m_compression, chunk.m_compressionMethod))
                    return Fail(context,
                                ArtifactMetadataErrorCode::kUnknownCompression,
                                "payloads.chunks.compression",
                                "unknown compression method");
                const bool hasNonzeroSizes = chunk.m_compressedSize > 0 && chunk.m_uncompressedSize > 0;
                if (!hasNonzeroSizes)
                    return Fail(context,
                                ArtifactMetadataErrorCode::kInvalidLayout,
                                "payloads.chunks",
                                "chunk sizes must be nonzero");
                if (chunk.m_uncompressedSize > kMaxPayloadSize)
                    return Fail(context,
                                ArtifactMetadataErrorCode::kLimitExceeded,
                                "payloads.chunks.uncompressedSize",
                                "chunk output exceeds the limit");
                if (chunk.m_offsetInPayload != expectedOffset)
                    return Fail(context,
                                ArtifactMetadataErrorCode::kInvalidLayout,
                                "payloads.chunks.offset",
                                "chunks must cover the payload contiguously");
                if (chunk.m_compressedSize > metadataPayload.m_size - expectedOffset)
                    return Fail(context,
                                ArtifactMetadataErrorCode::kInvalidLayout,
                                "payloads.chunks.compressedSize",
                                "chunk exceeds its payload source range");
                const bool isRawChunk = chunk.m_compressionMethod == Compression::Method::kNone;
                const bool hasValidRawSizes = !isRawChunk || chunk.m_compressedSize == chunk.m_uncompressedSize;
                if (!hasValidRawSizes)
                    return Fail(context,
                                ArtifactMetadataErrorCode::kInvalidLayout,
                                "payloads.chunks",
                                "uncompressed chunk sizes must match");
                if (outputSize > kMaxPayloadSize - chunk.m_uncompressedSize)
                    return Fail(context,
                                ArtifactMetadataErrorCode::kLimitExceeded,
                                "payloads.chunks.uncompressedSize",
                                "decoded payload exceeds the limit");
                expectedOffset += chunk.m_compressedSize;
                outputSize += chunk.m_uncompressedSize;
                payload.m_chunks.push_back(chunk);
            }
            if (expectedOffset != metadataPayload.m_size)
                return Fail(context,
                            ArtifactMetadataErrorCode::kInvalidLayout,
                            "payloads.chunks",
                            "chunks must cover the complete payload source range");
            result.m_payloads.push_back(std::move(payload));
        }
        return result;
    }

    void ArtifactStore::SetCatalogSource(const festd::string_view assetDirectoryPath)
    {
        GImpl->m_assetDirectoryPath = NormalizePath(GetAbsolutePath(assetDirectoryPath));
    }

    void ArtifactStore::SetPlatform(const festd::string_view platform)
    {
        GImpl->m_platform.assign(platform.data(), platform.size());
    }
} // namespace FE::IO
