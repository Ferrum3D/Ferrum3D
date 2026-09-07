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
        FE_RTTI_Reflect("78C4F7BD-AD22-4757-A6BB-A0959118FBE2");
        FE_RTTI_Serialize();

        AssetID m_assetId = AssetID::kNull;
        Rtti::TypeID m_expectedTypeId = Rtti::TypeID::kNull;
        DependencyKind m_kind = DependencyKind::kHard;
    };


    struct ArtifactChunkRecord final
    {
        FE_RTTI_Reflect("F54C7B65-1F16-4815-9652-A020D0E1A256");
        FE_RTTI_Serialize();

        uint64_t m_offsetInPayload = 0;
        uint64_t m_compressedSize = 0;
        uint64_t m_uncompressedSize = 0;
        Compression::Method m_compressionMethod = Compression::Method::kInvalid;
        Crc32 m_checksum;
    };


    struct ArtifactPayloadRecord final
    {
        FE_RTTI_Reflect("F4141F7E-F3B1-488E-B5CF-D6B2DAA3FF54");
        FE_RTTI_Serialize();

        ResolvedDataSource m_resolvedDataSource;
        festd::inline_vector<ArtifactChunkRecord> m_chunks;
    };


    struct ArtifactRecord final
    {
        FE_RTTI_Reflect("BFCD80E4-DB85-41F2-B43D-F08EA62AA6EC");
        FE_RTTI_Serialize();

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
        kIoError,
        kInvalidField,
        kUnsupportedSchema,
        kIdentityMismatch,
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
        festd::inline_string m_message;

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
        // The process environment owns the artifact store lifetime.
        static void Init();
        static void Shutdown();

        static ResolvedDataSource ResolveMeta(AssetID assetID);
        static ResolvedDataSource ResolveData(ArtifactID artifactID);
        static ArtifactDecodeResult Decode(festd::span<const std::byte> bytes, const ArtifactResolutionContext& context);
        static void SetCatalogSource(festd::string_view assetDirectoryPath);

    private:
        struct Impl;
        static Impl* GImpl;
    };
} // namespace FE::IO
