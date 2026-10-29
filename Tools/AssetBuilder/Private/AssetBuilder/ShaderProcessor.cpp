#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/AssetDepot.h>
#include <AssetBuilder/BuildState.h>
#include <AssetBuilder/PipelineInternal.h>
#include <AssetBuilder/ShaderProcessor.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Math/Uuid.h>
#include <Graphics/Core/ShaderSourceDirectory.h>

namespace FE::AssetBuilder
{
    using namespace Internal;

    namespace
    {
        bool HasPackageExtension(const ShaderPackageBuildSettings& settings, const festd::string_view path)
        {
            if (settings.m_extensions.empty())
                return true;

            const festd::string_view extension = IO::PathView{ path }.extension();
            for (const festd::string& candidate : settings.m_extensions)
            {
                if (festd::string_view{ candidate } == extension)
                    return true;
            }

            return false;
        }


        //! Read the next length-prefixed block written by AppendLogicalInput.
        bool ReadLogicalInput(const festd::span<const std::byte> data, uint32_t& offset, festd::string_view& result)
        {
            uint64_t size = 0;
            if (offset + sizeof(size) > data.size())
                return false;

            memcpy(&size, data.data() + offset, sizeof(size));
            offset += sizeof(size);
            if (size > data.size() - offset)
                return false;

            result = festd::string_view{ reinterpret_cast<const char*>(data.data() + offset), static_cast<uint32_t>(size) };
            offset += static_cast<uint32_t>(size);
            return true;
        }


        bool PublishShaderCache(const IO::Path& outputDirectory, Graphics::ShaderCacheAsset& asset)
        {
            // The artifact is staged like any build, then published with the data first and an atomic metadata replace.
            const IO::Path stagingDirectory = GetBuildDirectory(outputDirectory) / Fmt::FixedFormat("{}", Uuid::Random());
            const auto removeStaging = festd::defer([&stagingDirectory] {
                RemoveDirectoryTree(stagingDirectory);
            });

            ArtifactWriter writer{ stagingDirectory,
                                   Graphics::kShaderCacheAssetId,
                                   Rtti::GetTypeID<Graphics::ShaderCacheAsset>() };
            if (!writer.WriteHeader(asset) || !writer.Finish())
                return false;

            const IO::ArtifactID artifactId = writer.GetArtifactID();
            const IO::Path stagedData = ArtifactWriter::GetDataPath(stagingDirectory, artifactId);
            const IO::Path stagedMetadata = ArtifactWriter::GetMetadataPath(stagingDirectory, Graphics::kShaderCacheAssetId);
            const IO::Path finalData = ArtifactWriter::GetDataPath(outputDirectory, artifactId);
            const IO::Path finalMetadata = ArtifactWriter::GetMetadataPath(outputDirectory, Graphics::kShaderCacheAssetId);

            bool isPublished = IO::Directory::Create(IO::PathView{ finalData }.parent_directory()) == IO::ResultCode::kSuccess
                && IO::Directory::Create(IO::PathView{ finalMetadata }.parent_directory()) == IO::ResultCode::kSuccess;

            if (isPublished && !IO::File::Exists(finalData))
                isPublished = IO::File::Move(stagedData, finalData) == IO::ResultCode::kSuccess;

            if (isPublished)
                isPublished = IO::File::Replace(stagedMetadata, finalMetadata) == IO::ResultCode::kSuccess;

            IO::File::Delete(stagedData);
            IO::File::Delete(stagedMetadata);
            return isPublished;
        }
    } // namespace


    namespace Internal
    {
        bool ReadShaderPackageInputs(const ShaderPackageBuildSettings& settings, const IO::Path& sourceRoot,
                                     festd::vector<std::byte>& result)
        {
            IO::Path root;
            if (!ResolveSourceReference(sourceRoot, settings.m_rootDirectory, root))
                return false;

            festd::vector<IO::Path> files;
            IO::Directory::TraverseRecursively(root, [&files](const IO::DirectoryEntry& entry) {
                if (!Bit::AnySet(entry.m_attributes, IO::FileAttributeFlags::kDirectory))
                    files.push_back(IO::NormalizePath(festd::string_view{ entry.m_path }));

                return true;
            });

            // Sorting makes the package, and therefore its build key and artifact, independent of the traversal order.
            eastl::sort(files.begin(), files.end(), [](const IO::Path& lhs, const IO::Path& rhs) {
                return festd::string_view{ lhs.data(), lhs.size() } < festd::string_view{ rhs.data(), rhs.size() };
            });

            for (const IO::Path& file : files)
            {
                const festd::string_view path{ file.data(), file.size() };
                if (!HasPackageExtension(settings, path))
                    continue;

                const festd::string_view relativePath = path.substr_ascii(root.size() + 1);
                festd::vector<std::byte> contents;
                if (!ReadFile(file, contents))
                {
                    Logger::LogError("Failed to read shader source '{}'", file);
                    return false;
                }

                // Checkouts differ in line endings; normalizing them makes the package the same on every machine.
                contents.erase(eastl::remove(contents.begin(), contents.end(), std::byte{ '\r' }), contents.end());

                const auto* pathBytes = reinterpret_cast<const std::byte*>(relativePath.data());
                if (!AppendLogicalInput(result, festd::span{ pathBytes, relativePath.size() })
                    || !AppendLogicalInput(result, contents))
                {
                    return false;
                }
            }

            return true;
        }


        bool BuildShaderPackageArtifact(const BuildRequest& request)
        {
            const auto* settings = request.m_artifact.m_buildSettings.TryGet<ShaderPackageBuildSettings>();
            if (settings == nullptr)
            {
                Logger::LogError("Shader package product '{}' has incompatible build settings", request.m_artifact.m_name);
                return false;
            }

            Graphics::ShaderPackageAsset asset;
            asset.m_mounts.assign(settings->m_mounts.begin(), settings->m_mounts.end());

            // The source data holds the (path, contents) pairs read by ReadShaderPackageInputs.
            uint32_t offset = 0;
            while (offset < request.m_sourceData.size())
            {
                festd::string_view path;
                festd::string_view contents;
                if (!ReadLogicalInput(request.m_sourceData, offset, path)
                    || !ReadLogicalInput(request.m_sourceData, offset, contents))
                {
                    Logger::LogError("Shader package product '{}' has malformed inputs", request.m_artifact.m_name);
                    return false;
                }

                Graphics::ShaderPackageFile& file = asset.m_files.emplace_back();
                file.m_path = path;
                file.m_contents = contents;
            }

            ArtifactWriter writer(request.m_outputDirectory, request.m_artifact.m_assetId, request.m_artifact.m_assetTypeId);
            if (!writer.WriteHeader(asset) || !writer.Finish())
                return false;

            *request.m_resultArtifactId = writer.GetArtifactID();
            return true;
        }
    } // namespace Internal


    void AddDepotShaderSourceDirectories(const AssetDepot& depot)
    {
        for (const DepotAsset& asset : depot.GetAssets())
        {
            for (const AssetFileArtifact& artifact : asset.m_definition.m_artifacts)
            {
                const auto* settings = artifact.m_buildSettings.TryGet<ShaderPackageBuildSettings>();
                if (artifact.m_isRemoved || settings == nullptr || asset.m_sourceRoot.empty())
                    continue;

                IO::Path root;
                if (!ResolveSourceReference(asset.m_sourceRoot, settings->m_rootDirectory, root))
                    continue;

                for (const Graphics::ShaderPackageMount& mount : settings->m_mounts)
                {
                    const IO::Path directory = IO::NormalizePath(root / festd::string_view{ mount.m_directory });
                    Graphics::Core::AddShaderSourceDirectory(directory, festd::string_view{ mount.m_virtualPrefix });
                }
            }
        }
    }


    bool CompileShaderPermutations(const AssetDepot& depot,
                                   const festd::span<const Graphics::Core::ShaderPermutation> permutations,
                                   const Graphics::Core::ShaderCompilerConfig config,
                                   festd::vector<Graphics::Core::PrecompiledShader>* results)
    {
        FE_PROFILER_ZONE();

        AddDepotShaderSourceDirectories(depot);

        // A new compiler reads every source again, so edits made since an earlier compilation are seen.
        const Rc compiler = Graphics::Core::CreateShaderCompiler();

        festd::vector<Graphics::Core::PrecompiledShader> compiled;
        compiled.resize(permutations.size());
        festd::vector<uint8_t> succeeded;
        succeeded.resize(permutations.size(), 0);

        festd::vector<Rc<WaitGroup>> completions;
        for (uint32_t index = 0; index < permutations.size(); ++index)
        {
            completions.push_back(WaitGroup::Create());
            Jobs::DispatchForeground(
                [&permutations, &compiled, &succeeded, &compiler, config, index] {
                    festd::inline_vector<Graphics::Core::ShaderDefine, 4> defines;
                    Graphics::Core::ShaderCompilerArgs args;
                    permutations[index].GetCompilerArgs(args, defines);
                    args.m_config = config;

                    const Graphics::Core::ShaderCompilerResult result = compiler->CompileShader(args);
                    if (!result.m_codeValid)
                        return;

                    Graphics::Core::PrecompiledShader& shader = compiled[index];
                    shader.m_requestKey = Graphics::Core::ComputeShaderRequestKey(args);
                    shader.m_byteCodeSize = result.m_byteCodeSize;

                    const uint32_t wordCount = AlignUp<sizeof(uint32_t)>(result.m_byteCodeSize) / sizeof(uint32_t);
                    shader.m_byteCode.resize(wordCount);
                    memcpy(shader.m_byteCode.data(), result.m_byteCode.data(), wordCount * sizeof(uint32_t));

                    for (uint32_t dependency = 0; dependency < result.m_dependencies.size(); ++dependency)
                    {
                        Graphics::Core::PrecompiledShaderDependency& record = shader.m_dependencies.emplace_back();
                        record.m_path = result.m_dependencies[dependency];
                        record.m_contentHash = result.m_dependencyHashes[dependency];
                    }

                    succeeded[index] = 1;
                },
                completions.back().Get());
        }

        WaitGroup::WaitAll(festd::span<const Rc<WaitGroup>>{ completions.data(), completions.size() });

        uint32_t failedCount = 0;
        for (uint32_t index = 0; index < permutations.size(); ++index)
        {
            if (succeeded[index] == 0)
            {
                ++failedCount;
                continue;
            }

            if (results != nullptr)
                results->push_back(std::move(compiled[index]));
        }

        Logger::LogInfo("Compiled {} shader permutations, {} failed", permutations.size() - failedCount, failedCount);
        return failedCount == 0;
    }


    bool BuildShaderCache(const AssetDepot& depot, const Graphics::Core::ShaderCompilerConfig config)
    {
        FE_PROFILER_ZONE();

        const IO::Path& outputDirectory = depot.GetSettings().m_outputDirectory;
        const IO::Path manifestPath = Graphics::Core::GetShaderPermutationManifestPath(outputDirectory);

        festd::vector<Graphics::Core::ShaderPermutation> permutations;
        if (!Graphics::Core::LoadShaderPermutationManifest(manifestPath, permutations))
        {
            Logger::LogError("No shader permutation manifest at '{}'; run the executables that use the catalog first",
                             manifestPath);
            return false;
        }

        Graphics::ShaderCacheAsset asset;
        const bool isComplete =
            CompileShaderPermutations(depot, festd::span{ permutations.data(), permutations.size() }, config, &asset.m_shaders);

        // Order by request key, so that the same shaders always produce the same artifact.
        eastl::sort(asset.m_shaders.begin(),
                    asset.m_shaders.end(),
                    [](const Graphics::Core::PrecompiledShader& lhs, const Graphics::Core::PrecompiledShader& rhs) {
                        return lhs.m_requestKey < rhs.m_requestKey;
                    });

        if (!PublishShaderCache(outputDirectory, asset))
        {
            Logger::LogError("Failed to publish the shader cache to '{}'", outputDirectory);
            return false;
        }

        Logger::LogInfo("Published {} precompiled shaders as asset {}", asset.m_shaders.size(), Graphics::kShaderCacheAssetId);
        return isComplete;
    }
} // namespace FE::AssetBuilder
