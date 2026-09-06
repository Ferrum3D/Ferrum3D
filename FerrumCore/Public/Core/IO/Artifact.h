#pragma once
#include <Core/Compression/Compression.h>
#include <Core/IO/Assets.h>
#include <Core/Utils/Crc32.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct ArtifactChunkRecord final
    {
        FE_RTTI_Reflect("F54C7B65-1F16-4815-9652-A020D0E1A256");
        FE_RTTI_Serialize();

        uint64_t m_offsetInPayload;
        uint64_t m_uncompressedSize;

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
        festd::inline_vector<AssetID, 4> m_dependencies;
    };


    struct ArtifactStore final
    {
        static void Init();
        static void Shutdown();

        static ResolvedDataSource ResolveMeta(AssetID assetID);
        static void SetCatalogSource(festd::string_view assetDirectoryPath);

    private:
        struct Impl;
        static Impl* GImpl;
    };
} // namespace FE::IO
