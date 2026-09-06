#pragma once
#include <Core/Compression/Compression.h>
#include <Core/IO/Assets.h>
#include <Core/Utils/Crc32.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct ArtifactDependencyRecord final
    {
        AssetID m_assetId = AssetID::kNull;
        Rtti::TypeID m_expectedTypeId = Rtti::TypeID::kNull;
        DependencyKind m_kind = DependencyKind::kHard;
    };

    struct ArtifactChunkRecord final
    {
        uint64_t m_offsetInPayload = 0;
        uint64_t m_compressedSize = 0;
        uint64_t m_uncompressedSize = 0;
        Compression::Method m_compressionMethod = Compression::Method::kInvalid;
        Crc32 m_checksum;
    };


    struct ArtifactPayloadRecord final
    {
        ResolvedDataSource m_resolvedDataSource;
        festd::inline_vector<ArtifactChunkRecord> m_chunks;
    };


    struct ArtifactRecord final
    {
        ArtifactID m_artifactId = ArtifactID::kNull;
        AssetID m_assetId = AssetID::kNull;
        Rtti::TypeID m_assetTypeId = Rtti::TypeID::kNull;

        festd::inline_vector<ArtifactPayloadRecord, 1> m_payloads;
        festd::inline_vector<ArtifactDependencyRecord, 4> m_dependencies;
    };

    enum class ArtifactMetadataErrorCode : uint8_t
    {
        kNone,
        kInvalidJson,
        kMissingField,
        kInvalidField,
        kUnsupportedSchema,
        kIdentityMismatch,
        kPlatformMismatch,
        kUnknownType,
        kUnknownCompression,
        kInvalidLayout,
        kLimitExceeded,
    };

    struct ArtifactMetadataError final
    {
        ArtifactMetadataErrorCode m_code = ArtifactMetadataErrorCode::kNone;
        AssetID m_assetId = AssetID::kNull;
        Path m_source;
        festd::basic_fixed_string<128> m_field;
        festd::basic_fixed_string<256> m_message;

        [[nodiscard]] bool IsError() const
        {
            return m_code != ArtifactMetadataErrorCode::kNone;
        }
    };

    using ArtifactDecodeResult = festd::expected<ArtifactRecord, ArtifactMetadataError>;

    struct ArtifactResolutionContext final
    {
        AssetID m_assetId = AssetID::kNull;
        ArtifactID m_artifactId = ArtifactID::kNull;
        ResolvedDataSource m_metadataSource;
    };

    struct ArtifactStore final
    {
        static constexpr uint32_t kMetadataVersion = 1;

        // The process environment owns the artifact store lifetime.
        static void Init();
        static void Shutdown();

        static ResolvedDataSource ResolveMeta(AssetID assetID);
        static ResolvedDataSource ResolveData(ArtifactID artifactID);
        static ArtifactDecodeResult Decode(festd::span<const std::byte> bytes, const ArtifactResolutionContext& context);
        static void SetCatalogSource(festd::string_view assetDirectoryPath);
        static void SetPlatform(festd::string_view platform);

    private:
        struct Impl;
        static Impl* GImpl;
    };
} // namespace FE::IO
