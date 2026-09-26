#include <AssetBuilder/PipelineInternal.h>
#include <Core/Base/PlatformInclude.h>
#include <Core/IO/FileStream.h>
#include <Core/Logging/Logger.h>
#include <bcrypt.h>

namespace FE::AssetBuilder
{
    namespace Internal
    {
        bool ReadFile(const IO::Path& path, festd::vector<std::byte>& bytes)
        {
            auto fileResult = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
            if (!fileResult)
                return false;

            const Rc<IO::FileStream> stream = *fileResult;
            const size_t size = stream->Length();
            if (size > Constants::kMaxU32)
                return false;

            bytes.resize(static_cast<uint32_t>(size));
            return stream->ReadToBuffer(bytes.data(), bytes.size()) == size;
        }


        bool AppendLogicalInput(festd::vector<std::byte>& result, const festd::span<const std::byte> bytes)
        {
            const uint32_t oldSize = result.size();
            if (oldSize > Constants::kMaxU32 - sizeof(uint64_t) || bytes.size() > Constants::kMaxU32 - oldSize - sizeof(uint64_t))
                return false;

            const uint64_t byteSize = bytes.size();
            result.resize(oldSize + sizeof(byteSize) + bytes.size());
            memcpy(result.data() + oldSize, &byteSize, sizeof(byteSize));
            memcpy(result.data() + oldSize + sizeof(byteSize), bytes.data(), bytes.size());
            return true;
        }


        bool PathsHaveEqualPrefix(const festd::string_view lhs, const festd::string_view rhs)
        {
            if (lhs.size() < rhs.size())
                return false;

            for (uint32_t index = 0; index < rhs.size(); ++index)
            {
                if (ASCII::ToLower(lhs.byte_at(index)) != ASCII::ToLower(rhs.byte_at(index)))
                    return false;
            }

            return true;
        }


        bool MakeSourceReference(const IO::Path& sourceRoot, const IO::Path& sourcePath, IO::Path& result)
        {
            const festd::string_view root = sourceRoot;
            const festd::string_view source = sourcePath;
            if (root.empty() || source.size() <= root.size() || !PathsHaveEqualPrefix(source, root))
                return false;

            uint32_t relativeOffset = root.size();
            if (!PathParser::IsPathSeparator(root.byte_at(root.size() - 1)))
            {
                if (!PathParser::IsPathSeparator(source.byte_at(relativeOffset)))
                    return false;
                ++relativeOffset;
            }

            result = source.substr(relativeOffset);
            return !result.empty();
        }


        bool FindSourceRoot(const IO::Path& sourcePath, IO::Path& result)
        {
            IO::Path directory = IO::NormalizePath(IO::PathView(sourcePath).parent_directory());
            while (!directory.empty())
            {
                if (IO::File::Exists(directory / kSourceDepotRootMarker))
                {
                    result = directory;
                    return true;
                }

                const IO::Path parent = IO::NormalizePath(IO::PathView(directory).parent_directory());
                if (parent == directory)
                    break;
                directory = parent;
            }

            Logger::LogError("Source '{}' is not inside a depot marked by '{}'", sourcePath, kSourceDepotRootMarker);
            return false;
        }


        bool ResolveSourceReference(const IO::Path& sourceRoot, const IO::Path& reference, IO::Path& result)
        {
            if (reference.empty() || IO::PathView(reference).is_absolute())
            {
                Logger::LogError("Source reference '{}' must be relative to the source depot root", reference);
                return false;
            }

            result = IO::NormalizePath(sourceRoot / reference);
            IO::Path verifiedReference;
            if (MakeSourceReference(sourceRoot, result, verifiedReference))
                return true;

            Logger::LogError("Source reference '{}' escapes depot root '{}'", reference, sourceRoot);
            return false;
        }


        IO::AssetID GenerateAssetId()
        {
            IO::AssetID result{ kForceInit };
            const NTSTATUS status =
                BCryptGenRandom(nullptr, result.data(), static_cast<ULONG>(result.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
            if (status < 0)
                return IO::AssetID::kNull;

            result.m_bytes[6] = (result.m_bytes[6] & 0x0f) | 0x40;
            result.m_bytes[8] = (result.m_bytes[8] & 0x3f) | 0x80;
            return result;
        }


        const AssetFileArtifact* FindPreviousArtifact(const AssetFile* previous, const festd::string_view productKey,
                                                      const festd::string_view legacyName, const Rtti::TypeID typeId)
        {
            if (previous == nullptr)
                return nullptr;

            for (const AssetFileArtifact& artifact : previous->m_artifacts)
            {
                const bool productKeyMatches = artifact.m_productKey == productKey;
                const bool legacyNameMatches = artifact.m_productKey.empty() && artifact.m_name == legacyName;
                if ((productKeyMatches || legacyNameMatches) && artifact.m_assetTypeId == typeId)
                    return &artifact;
            }

            return nullptr;
        }


        bool InitializeArtifact(AssetFileArtifact& artifact, const AssetFile* previous, const festd::string_view productKey,
                                const festd::string_view name, const Rtti::TypeID typeId)
        {
            artifact.m_productKey = productKey;
            artifact.m_name = name;
            artifact.m_assetTypeId = typeId;
            if (const AssetFileArtifact* oldArtifact = FindPreviousArtifact(previous, productKey, name, typeId))
            {
                artifact.m_assetId = oldArtifact->m_assetId;
                artifact.m_buildSettings = oldArtifact->m_buildSettings;
                artifact.m_lastBuildKey = oldArtifact->m_lastBuildKey;
                artifact.m_builtArtifactId = oldArtifact->m_builtArtifactId;
            }
            else
            {
                artifact.m_assetId = GenerateAssetId();
            }

            if (artifact.m_assetId.IsValid())
                return true;

            Logger::LogError("Failed to generate an asset ID for product '{}'", name);
            return false;
        }


        bool HasArtifact(const AssetFile& assetFile, const festd::string_view productKey, const Rtti::TypeID typeId)
        {
            for (const AssetFileArtifact& artifact : assetFile.m_artifacts)
            {
                if (artifact.m_productKey == productKey && artifact.m_assetTypeId == typeId)
                    return true;
            }

            return false;
        }


        void AppendRemovedArtifacts(AssetFile& assetFile, const AssetFile* previous)
        {
            if (previous == nullptr)
                return;

            for (const AssetFileArtifact& oldArtifact : previous->m_artifacts)
            {
                bool wasRediscovered = false;
                for (const AssetFileArtifact& artifact : assetFile.m_artifacts)
                    wasRediscovered |= artifact.m_assetId == oldArtifact.m_assetId;
                if (wasRediscovered)
                    continue;

                AssetFileArtifact& removedArtifact = assetFile.m_artifacts.emplace_back(oldArtifact);
                removedArtifact.m_dependencies.clear();
                removedArtifact.m_isRemoved = true;
            }
        }


        void AddDependency(AssetFileArtifact& artifact, const AssetFileArtifact& dependency)
        {
            AssetFileDependency& result = artifact.m_dependencies.emplace_back();
            result.m_assetId = dependency.m_assetId;
            result.m_expectedTypeId = dependency.m_assetTypeId;
        }


        bool LoadPreviousAssetFile(const IO::Path& path, AssetFile& previous, const AssetFile*& previousPtr)
        {
            previousPtr = nullptr;
            if (!IO::File::Exists(path))
                return true;

            if (!LoadAssetFile(path, previous))
            {
                Logger::LogError("Failed to read existing asset file '{}'", path);
                return false;
            }

            previousPtr = &previous;
            return true;
        }
    } // namespace Internal
} // namespace FE::AssetBuilder
