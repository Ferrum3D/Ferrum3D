#include <Core/IO/Artifact.h>
#include <Core/IO/StreamBase.h>
#include <Core/RTTI/Reflection.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Core/Serialization/Serialization.h>

namespace FE::IO
{
    namespace
    {
        //! Construct a decode failure using the caller's requested identity and exact metadata source.
        ArtifactDecodeResult Fail(const ArtifactResolutionContext& context, const ArtifactMetadataErrorCode code,
                                  const festd::string_view message)
        {
            ArtifactMetadataError error;
            error.m_code = code;
            error.m_assetId = context.m_assetId;
            error.m_source = context.m_metadataSource.m_filePath;
            error.m_message.assign(message.data(), message.size());
            return festd::unexpected(std::move(error));
        }


        //! True when a serialized payload override stays within the configured artifact-store root.
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


        //! Convert serialization-layer failures into the smaller artifact metadata error vocabulary.
        ArtifactMetadataErrorCode MapSerializationError(const Serialization::ResultCode result)
        {
            if (result == Serialization::ResultCode::kTypeMismatch)
                return ArtifactMetadataErrorCode::kUnsupportedSchema;

            return ArtifactMetadataErrorCode::kInvalidFormat;
        }
    } // namespace


    //! Process-owned development artifact-store configuration.
    struct ArtifactStore::Impl final
    {
        //! Absolute root prepended to deterministic metadata/data paths and safe relative payload overrides.
        Path m_assetDirectoryPath;
    };

    //! Active process-wide implementation, owned by Env initialization/shutdown.
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
        // Deterministic sharded metadata lookup: artifacts/metadata/<platform>/<2>/<2>/<asset-id>.meta.
        const auto idString = Str::ToLower<festd::fixed_string>(Fmt::FixedFormat("{}", assetID));
        Path relativePath("artifacts/metadata/pc");
        relativePath /= festd::string_view{ idString.data(), 2 };
        relativePath /= festd::string_view{ idString.data() + 2, 2 };
        Path fileName(idString);
        fileName.append(".meta");
        relativePath /= fileName;
        return ResolvedDataSource{ GImpl->m_assetDirectoryPath / relativePath };
    }


    ResolvedDataSource ArtifactStore::ResolveData(const ArtifactID artifactID)
    {
        // Immutable artifact-addressed payload lookup: artifacts/data/<2>/<2>/<artifact-id>.bin.
        const auto idString = Str::ToLower<festd::fixed_string>(Fmt::FixedFormat("{}", artifactID));
        Path relativePath("artifacts/data");
        relativePath /= festd::string_view{ idString.data(), 2 };
        relativePath /= festd::string_view{ idString.data() + 2, 2 };
        Path fileName(idString);
        fileName.append(".bin");
        relativePath /= fileName;
        return ResolvedDataSource{ GImpl->m_assetDirectoryPath / relativePath };
    }


    ArtifactDecodeResult ArtifactStore::Decode(const festd::span<const std::byte> bytes, const ArtifactResolutionContext& context)
    {
        // Decode stage: ArtifactStore, rather than AssetManager, owns selection and use of the metadata serialization format.
        ReadOnlyMemoryStream stream(bytes);
        Serialization::JsonFormat format;
        Serialization::DeserializationContext deserializationContext(&stream, format);

        ArtifactRecord result;
        const Serialization::ResultCode deserializeResult = deserializationContext.Load(result);
        if (deserializeResult != Serialization::ResultCode::kSuccess)
        {
            const ArtifactMetadataErrorCode code = MapSerializationError(deserializeResult);
            return Fail(context, code, "metadata does not match the JSON artifact schema");
        }

        // Identity/type stage: reject valid JSON that belongs to another request or cannot be constructed by this runtime.
        if (result.m_assetId != context.m_assetId)
            return Fail(context, ArtifactMetadataErrorCode::kIdentityMismatch, "asset does not match the requested asset");

        if (context.m_artifactId.IsValid() && result.m_artifactId != context.m_artifactId)
            return Fail(context, ArtifactMetadataErrorCode::kIdentityMismatch, "artifact does not match the requested artifact");

        if (Rtti::TypeRegistry::FindType(result.m_assetTypeId) == nullptr)
            return Fail(context, ArtifactMetadataErrorCode::kUnknownType, "asset type ID is not registered");

        // Layout stage: payload zero is the mandatory generic serialized-object payload.
        if (result.m_payloads.empty())
            return Fail(context, ArtifactMetadataErrorCode::kInvalidFormat, "payload zero is required");

        // Normalization stage: replace implicit artifact data paths and anchor safe overrides at the configured store root.
        for (ArtifactPayloadRecord& payload : result.m_payloads)
        {
            ResolvedDataSource& dataSource = payload.m_resolvedDataSource;
            if (dataSource.m_filePath.empty())
            {
                dataSource.m_filePath = ResolveData(result.m_artifactId).m_filePath;
            }
            else
            {
                if (!IsSafeRelativeSource(dataSource.m_filePath))
                    return Fail(context, ArtifactMetadataErrorCode::kInvalidFormat, "payload source must be store-relative");

                dataSource.m_filePath = GImpl->m_assetDirectoryPath / dataSource.m_filePath;
            }
        }

        return result;
    }


    void ArtifactStore::SetCatalogSource(const festd::string_view assetDirectoryPath)
    {
        GImpl->m_assetDirectoryPath = GetAbsolutePath(assetDirectoryPath);
    }
} // namespace FE::IO
