#pragma once
#include <Core/Compression/Compression.h>
#include <Core/IO/Assets.h>
#include <Core/Utils/Crc32.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::IO
{
    //! @brief One direct logical dependency declared by artifact metadata.
    struct ArtifactDependencyRecord final
    {
        FE_RTTI_Reflect("78C4F7BD-AD22-4757-A6BB-A0959118FBE2");
        FE_RTTI_Serialize();

        //! Target logical asset identity.
        AssetID m_assetId = AssetID::kNull;

        //! Runtime type required at the target; discovery fails if decoded target metadata disagrees.
        Rtti::TypeID m_expectedTypeId = Rtti::TypeID::kNull;

        //! Whether the target participates in automatic residency discovery.
        DependencyKind m_kind = DependencyKind::kHard;
    };


    //! @brief One independently compressed physical block within a payload source range.
    //!
    //! Chunk order defines output concatenation order. Offsets are relative to the payload's ResolvedDataSource range, and the
    //! checksum covers the uncompressed bytes.
    struct ArtifactChunkRecord final
    {
        FE_RTTI_Reflect("F54C7B65-1F16-4815-9652-A020D0E1A256");
        FE_RTTI_Serialize();

        //! Compressed source offset relative to the payload range.
        uint64_t m_offsetInPayload = 0;

        //! Number of bytes read from the physical source.
        uint64_t m_compressedSize = 0;

        //! Exact number of bytes expected after decompression.
        uint64_t m_uncompressedSize = 0;

        //! Codec used for this chunk; kNone/raw is represented by the compression subsystem's raw method.
        Compression::Method m_compressionMethod = Compression::Method::kInvalid;

        //! Integrity checksum of the uncompressed chunk bytes.
        Crc32 m_checksum;
    };


    //! @brief Independently requestable logical payload backed by one resolved file range.
    //!
    //! Payload zero contains the serialized primary object. Additional payloads are reserved for type-specific streaming data.
    struct ArtifactPayloadRecord final
    {
        FE_RTTI_Reflect("F4141F7E-F3B1-488E-B5CF-D6B2DAA3FF54");
        FE_RTTI_Serialize();

        //! Physical file and bounded byte range containing every chunk in this payload.
        ResolvedDataSource m_resolvedDataSource;

        //! Ordered compression blocks whose decoded output forms the payload bytes.
        festd::inline_vector<ArtifactChunkRecord> m_chunks;
    };


    //! @brief Normalized metadata for one immutable compiled representation of a logical asset.
    //!
    //! ArtifactStore deserializes this record, validates its identity and layout, and resolves all payload sources before exposing
    //! it to AssetManager. The record is then pinned by the shared load operation for consistent discovery.
    struct ArtifactRecord final
    {
        FE_RTTI_Reflect("BFCD80E4-DB85-41F2-B43D-F08EA62AA6EC");
        FE_RTTI_Serialize();

        //! Identity of this immutable compiled representation.
        ArtifactID m_artifactId = ArtifactID::kNull;

        //! Logical asset identity represented by this artifact.
        AssetID m_assetId = AssetID::kNull;

        //! Runtime type used to construct and deserialize the primary payload.
        Rtti::TypeID m_assetTypeId = Rtti::TypeID::kNull;

        //! Independently requestable payloads; element zero is required.
        festd::inline_vector<ArtifactPayloadRecord, 1> m_payloads;

        //! Direct typed dependency edges. Transitive closure is discovered by AssetManager.
        festd::inline_vector<ArtifactDependencyRecord, 4> m_dependencies;
    };


    //! @brief Structured reason that artifact metadata could not be read, decoded, normalized, or validated.
    enum class ArtifactMetadataErrorCode : uint8_t
    {
        kNone,
        kInvalidFormat,
        kIoError,
        kUnsupportedSchema,
        kIdentityMismatch,
        kUnknownType,
    };


    //! @brief Metadata failure with enough source context for diagnostics.
    struct ArtifactMetadataError final
    {
        //! Logical asset whose metadata was requested.
        AssetID m_assetId = AssetID::kNull;

        //! Machine-readable failure category; kNone means no error.
        ArtifactMetadataErrorCode m_code = ArtifactMetadataErrorCode::kNone;

        //! Resolved metadata source that produced the failure.
        Path m_source;

        //! Concise human-readable detail suitable for logs and tools.
        festd::inline_string m_message;

        //! @brief True when m_code identifies a failure.
        [[nodiscard]] bool IsError() const
        {
            return m_code != ArtifactMetadataErrorCode::kNone;
        }
    };


    //! Result of decoding and normalizing one metadata document.
    using ArtifactDecodeResult = festd::expected<ArtifactRecord, ArtifactMetadataError>;

    //! @brief Expected identity and physical source supplied by the caller when decoding metadata.
    //!
    //! Comparing decoded identities with this context prevents bytes read for one request from being accepted as another asset or
    //! artifact.
    struct ArtifactResolutionContext final
    {
        //! Required logical identity requested by AssetManager.
        AssetID m_assetId = AssetID::kNull;

        //! Optional required artifact identity; null accepts the artifact selected by development metadata.
        ArtifactID m_artifactId = ArtifactID::kNull;

        //! Exact file/range from which the metadata bytes originated.
        ResolvedDataSource m_metadataSource;
    };


    //! @brief Resolves deterministic development paths and owns the artifact metadata encoding contract.
    //!
    //! The store resolves and validates locations but does not execute I/O. Callers read bytes through the asynchronous I/O layer
    //! and pass them to Decode. The process environment owns initialization and shutdown.
    struct ArtifactStore final
    {
        //! @brief Initialize process-wide store state.
        static void Init();

        //! @brief Destroy process-wide store state after all users have stopped.
        static void Shutdown();

        //! @brief Resolve the deterministic platform metadata location for a logical asset.
        static ResolvedDataSource ResolveMeta(AssetID assetID);

        //! @brief Resolve the deterministic immutable data location for an artifact.
        static ResolvedDataSource ResolveData(ArtifactID artifactID);

        //! @brief Decode, validate, and normalize metadata bytes using their request/source context.
        static ArtifactDecodeResult Decode(festd::span<const std::byte> bytes, const ArtifactResolutionContext& context);

        //! @brief Configure the development artifact root used by subsequent resolution and decode calls.
        //!
        //! The root is converted to an absolute path and must remain unchanged while asset operations are active.
        static void SetCatalogSource(festd::string_view assetDirectoryPath);

    private:
        struct Impl;
        static Impl* GImpl;
    };
} // namespace FE::IO
