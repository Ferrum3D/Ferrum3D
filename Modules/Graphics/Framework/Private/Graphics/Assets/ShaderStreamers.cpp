#include <Graphics/Assets/ShaderStreamers.h>
#include <festd/unordered_map.h>

namespace FE::Graphics
{
    namespace
    {
        //! Serves the files of a published package. The asset owns the files and outlives the mount, see OnAssetDestroyed.
        struct PackageSources final : public Core::ShaderSourcePackage
        {
            explicit PackageSources(const ShaderPackageAsset& asset)
                : m_asset(asset)
            {
                for (uint32_t index = 0; index < asset.m_files.size(); ++index)
                    m_fileIndices.insert({ DefaultHash(festd::string_view{ asset.m_files[index].m_path }), index });
            }

            bool ReadFile(const festd::string_view virtualPath, festd::pmr::vector<std::byte>& contents) const override
            {
                for (const ShaderPackageMount& mount : m_asset.m_mounts)
                {
                    const festd::string_view prefix{ mount.m_virtualPrefix };
                    if (!virtualPath.starts_with(prefix))
                        continue;

                    const festd::string packagePath =
                        Fmt::Format("{}{}", mount.m_directory, virtualPath.substr_ascii(prefix.size()));
                    const ShaderPackageFile* file = FindFile(festd::string_view{ packagePath });
                    if (file == nullptr)
                        continue;

                    const auto* bytes = reinterpret_cast<const std::byte*>(file->m_contents.data());
                    contents.assign(bytes, bytes + file->m_contents.size());
                    return true;
                }

                return false;
            }

        private:
            [[nodiscard]] const ShaderPackageFile* FindFile(const festd::string_view path) const
            {
                const auto iter = m_fileIndices.find(DefaultHash(path));
                if (iter == m_fileIndices.end())
                    return nullptr;

                const ShaderPackageFile& file = m_asset.m_files[iter->second];
                return festd::string_view{ file.m_path } == path ? &file : nullptr;
            }

            void DoRelease() override
            {
                Memory::DefaultDelete(this);
            }

            const ShaderPackageAsset& m_asset;
            festd::unordered_dense_map<uint64_t, uint32_t> m_fileIndices;
        };
    } // namespace


    IO::AssetFinalizeResult ShaderPackageStreamer::FinalizeAssetLoading(IO::AssetSlot& assetSlot, const IO::ArtifactRecord&,
                                                                        void* candidate)
    {
        auto* asset = static_cast<ShaderPackageAsset*>(candidate);
        asset->m_runtime = Memory::DefaultNew<PackageSources>(*asset);

        // A reload replaces the previous generation's mount, which reports every cached source as possibly changed.
        Core::MountShaderSourcePackage(assetSlot.m_assetId, asset->m_runtime.Get());
        return IO::AssetFinalizeResult::kSucceeded;
    }


    IO::AssetFinalizeResult ShaderPackageStreamer::PollFinalize(IO::AssetSlot&, void*)
    {
        return IO::AssetFinalizeResult::kSucceeded;
    }


    void ShaderPackageStreamer::CancelFinalize(IO::AssetSlot& assetSlot, void* candidate)
    {
        OnAssetDestroyed(assetSlot, candidate);
    }


    void ShaderPackageStreamer::OnAssetDestroyed(IO::AssetSlot& assetSlot, void* asset)
    {
        // Unmounting waits for reads in progress, so the files can be destroyed with the asset afterwards.
        auto* package = static_cast<ShaderPackageAsset*>(asset);
        if (package->m_runtime)
            Core::UnmountShaderSourcePackage(assetSlot.m_assetId, package->m_runtime.Get());

        package->m_runtime.Reset();
    }


    IO::AssetFinalizeResult ShaderCacheStreamer::FinalizeAssetLoading(IO::AssetSlot&, const IO::ArtifactRecord&, void* candidate)
    {
        auto* asset = static_cast<ShaderCacheAsset*>(candidate);
        asset->m_runtime = Memory::DefaultNew<Core::PrecompiledShaderSet>(std::move(asset->m_shaders));
        asset->m_shaders.clear();

        Core::SetPrecompiledShaders(asset->m_runtime.Get());
        Logger::LogInfo("Loaded {} precompiled shaders", asset->m_runtime->GetShaderCount());
        return IO::AssetFinalizeResult::kSucceeded;
    }


    IO::AssetFinalizeResult ShaderCacheStreamer::PollFinalize(IO::AssetSlot&, void*)
    {
        return IO::AssetFinalizeResult::kSucceeded;
    }


    void ShaderCacheStreamer::CancelFinalize(IO::AssetSlot& assetSlot, void* candidate)
    {
        OnAssetDestroyed(assetSlot, candidate);
    }


    void ShaderCacheStreamer::OnAssetDestroyed(IO::AssetSlot&, void* asset)
    {
        auto* shaderCache = static_cast<ShaderCacheAsset*>(asset);
        if (shaderCache->m_runtime && Core::GetPrecompiledShaders() == shaderCache->m_runtime)
            Core::SetPrecompiledShaders(nullptr);

        shaderCache->m_runtime.Reset();
    }
} // namespace FE::Graphics
