#include <AssetBuilder/AssetDepot.h>
#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/BuildState.h>
#include <AssetBuilder/ShaderProcessor.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/FileStream.h>
#include <Core/Math/Uuid.h>
#include <Core/Threading/Thread.h>
#include <Graphics/Assets/ShaderStreamers.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/PrecompiledShaders.h>
#include <Graphics/Core/ShaderPermutationManifest.h>
#include <Graphics/Core/ShaderSourceDirectory.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/PipelineFactory.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

namespace
{
    constexpr festd::string_view kPackageInitialShader = "[numthreads(1, 1, 1)] void main() {}\n";
    constexpr festd::string_view kPackageEditedShader = "// edited\n[numthreads(1, 1, 1)] void main() {}\n";


    //! A package with a single file, standing in for a published shader package.
    struct SingleFilePackage final : public Core::ShaderSourcePackage
    {
        SingleFilePackage(const Env::Name path, const festd::string_view contents)
            : m_path(path)
            , m_contents(contents)
        {
        }

        bool ReadFile(const festd::string_view virtualPath, festd::pmr::vector<std::byte>& contents) const override
        {
            if (virtualPath != festd::string_view{ m_path })
                return false;

            const auto* bytes = reinterpret_cast<const std::byte*>(m_contents.data());
            contents.assign(bytes, bytes + m_contents.size());
            return true;
        }

    private:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        Env::Name m_path;
        festd::string m_contents;
    };


    //! A virtual path that exists in no registered directory, so only a package can provide it.
    Env::Name MakePackageShaderPath()
    {
        return Env::Name{ Fmt::FixedFormat("ShaderAssetsTests/{}/Probe.cs.hlsl", Uuid::Random()) };
    }


    void WriteSourceText(const IO::Path& path, const festd::string_view text)
    {
        ASSERT_EQ(IO::Directory::Create(IO::PathView{ path }.parent_directory()), IO::ResultCode::kSuccess);

        auto stream = IO::FileStream::Open(path, IO::OpenMode::kCreate);
        ASSERT_TRUE(stream);
        ASSERT_EQ(stream.value()->WriteFromBuffer(text.data(), text.size()), text.size());
        stream.value()->Close();
    }


    festd::string ReadSourceText(const IO::Path& path)
    {
        festd::string text;
        auto stream = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
        if (!stream)
            return text;

        const uint32_t size = static_cast<uint32_t>(stream.value()->Length());
        text.resize(size, ' ');
        stream.value()->ReadToBuffer(text.data(), size);
        return text;
    }


    Core::ShaderCompilerArgs MakePackageComputeArgs(const Env::Name path)
    {
        Core::ShaderCompilerArgs args;
        args.m_shaderName = path;
        args.m_stage = Core::ShaderStage::kCompute;
        return args;
    }
} // namespace


TEST(ShaderAssets, MountedPackageServesSourcesAndHotReloadsPipelines)
{
    const Env::Name shaderPath = MakePackageShaderPath();
    const IO::AssetID packageKey = Uuid::Random();
    Core::MountShaderSourcePackage(packageKey, Memory::DefaultNew<SingleFilePackage>(shaderPath, kPackageInitialShader));

    Core::DeviceFactory::Init(Core::GraphicsApi::kVulkan);
    const auto shutdown = festd::defer([] {
        Core::DeviceFactory::Shutdown();
    });

    Core::DeviceFactory& deviceFactory = Core::DeviceFactory::Get();
    const auto adapters = deviceFactory.EnumerateAdapters();
    ASSERT_FALSE(adapters.empty());

    Rc<Core::Device> device = deviceFactory.CreateDevice(adapters.front().m_name);
    {
        Rc<Core::DescriptorManager> descriptors = device->CreateDescriptorManager();
        Rc<Core::PipelineFactory> pipelines = device->CreatePipelineFactory(descriptors.Get());
        auto& factory = *Rtti::AssertCast<Vulkan::PipelineFactory*>(pipelines.Get());

        Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader(shaderPath);
        Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(pipeline->IsReady());

        factory.UpdateShaderHotReload();
        const VkPipeline initialNative = Vulkan::ImplCast(pipeline)->GetNative();

        // Replacing the package, as a reloaded package asset does, recompiles the shaders that use its files.
        Core::MountShaderSourcePackage(packageKey, Memory::DefaultNew<SingleFilePackage>(shaderPath, kPackageEditedShader));

        bool isReplaced = false;
        for (uint32_t attempt = 0; attempt < 1000 && !isReplaced; ++attempt)
        {
            factory.UpdateShaderHotReload();
            isReplaced = Vulkan::ImplCast(pipeline)->GetNative() != initialNative;
            Threading::Sleep(10);
        }

        EXPECT_TRUE(isReplaced);
        EXPECT_TRUE(pipeline->IsReady());
    }

    device->WaitIdle();
}


TEST(ShaderAssets, PrecompiledShadersAreUsedWhileTheirSourcesMatch)
{
    const Env::Name shaderPath = MakePackageShaderPath();
    const IO::AssetID packageKey = Uuid::Random();
    Core::MountShaderSourcePackage(packageKey, Memory::DefaultNew<SingleFilePackage>(shaderPath, kPackageInitialShader));
    const auto cleanup = festd::defer([] {
        Core::SetPrecompiledShaders(nullptr);
    });

    const Core::ShaderCompilerArgs args = MakePackageComputeArgs(shaderPath);
    const Core::ShaderCompilerResult compiled = Core::CreateShaderCompiler()->CompileShader(args);
    ASSERT_TRUE(compiled.m_codeValid);
    ASSERT_EQ(compiled.m_dependencies.size(), compiled.m_dependencyHashes.size());

    // A recognizable result proves that the precompiled shader was used instead of the compiler or the persistent cache.
    festd::vector<Core::PrecompiledShader> shaders;
    Core::PrecompiledShader& shader = shaders.emplace_back();
    shader.m_requestKey = Core::ComputeShaderRequestKey(args);
    shader.m_byteCode = { 0x07230203u, 0x12345678u };
    shader.m_byteCodeSize = 8;
    for (uint32_t index = 0; index < compiled.m_dependencies.size(); ++index)
        shader.m_dependencies.push_back({ compiled.m_dependencies[index], compiled.m_dependencyHashes[index] });

    Core::SetPrecompiledShaders(Memory::DefaultNew<Core::PrecompiledShaderSet>(std::move(shaders)));

    const Core::ShaderCompilerResult precompiled = Core::CreateShaderCompiler()->CompileShader(args);
    ASSERT_TRUE(precompiled.m_codeValid);
    ASSERT_EQ(precompiled.m_byteCodeSize, 8u);
    EXPECT_EQ(reinterpret_cast<const uint32_t*>(precompiled.m_byteCode.data())[1], 0x12345678u);

    // Once the source changes, the precompiled shader is stale and the compiler takes over again.
    Core::MountShaderSourcePackage(packageKey, Memory::DefaultNew<SingleFilePackage>(shaderPath, kPackageEditedShader));
    const Core::ShaderCompilerResult recompiled = Core::CreateShaderCompiler()->CompileShader(args);
    ASSERT_TRUE(recompiled.m_codeValid);
    EXPECT_NE(recompiled.m_byteCodeSize, 8u);
}


TEST(ShaderAssets, ManifestRecordsEachPermutationOnce)
{
    const IO::Path catalog = IO::Directory::GetExecutableDirectory() / Fmt::FixedFormat("ShaderAssetsTests/{}", Uuid::Random());
    IO::ArtifactStore::SetCatalogSource(catalog);
    const auto cleanup = festd::defer([&catalog] {
        IO::ArtifactStore::SetCatalogSource({});
        AssetBuilder::RemoveDirectoryTree(catalog);
    });

    const Core::ShaderDefine defines[] = { { "LOD_COUNT", "4" }, { "USE_HALF", "1" } };
    Core::ShaderCompilerArgs args = MakePackageComputeArgs(Env::Name{ "Shaders/Tests/Manifest.cs.hlsl" });
    args.m_defines = defines;
    Core::RecordShaderPermutation(args);
    Core::RecordShaderPermutation(args);

    const IO::Path manifestPath = Core::GetShaderPermutationManifestPath(catalog);
    EXPECT_EQ(ReadSourceText(manifestPath), festd::string_view{ "cs Shaders/Tests/Manifest.cs.hlsl LOD_COUNT=4 USE_HALF=1\n" });

    festd::vector<Core::ShaderPermutation> permutations;
    ASSERT_TRUE(Core::LoadShaderPermutationManifest(manifestPath, permutations));
    ASSERT_EQ(permutations.size(), 1u);

    festd::inline_vector<Core::ShaderDefine, 4> loadedDefines;
    Core::ShaderCompilerArgs loadedArgs;
    permutations[0].GetCompilerArgs(loadedArgs, loadedDefines);
    EXPECT_EQ(Core::ComputeShaderRequestKey(loadedArgs), Core::ComputeShaderRequestKey(args));
}


TEST(ShaderAssets, PackageBuiltFromSourcesIsMountedByItsStreamer)
{
    const IO::Path depot = IO::Directory::GetExecutableDirectory() / Fmt::FixedFormat("ShaderAssetsTests/{}", Uuid::Random());
    const auto cleanup = festd::defer([&depot] {
        IO::ArtifactStore::SetCatalogSource({});
        AssetBuilder::RemoveDirectoryTree(depot);
    });

    // Shaders include each other by virtual path, so the include names the package's mount prefix.
    const festd::fixed_string prefix = Fmt::FixedFormat("ShaderAssetsTests/{}/", Uuid::Random());
    const festd::fixed_string probe = Fmt::FixedFormat("#include \"{}Common/Value.hlsli\"\n", prefix);
    WriteSourceText(depot / ".ferrum-source-depot-root", "");
    WriteSourceText(depot / "Shaders/Probe.cs.hlsl", festd::string_view{ probe.data(), probe.size() });
    WriteSourceText(depot / "Shaders/Common/Value.hlsli", "#define VALUE 1\n[numthreads(1, 1, 1)] void main() {}\n");
    WriteSourceText(depot / "Shaders/Notes.txt", "not a shader");
    const IO::AssetID packageId = Uuid::Random();
    {
        AssetBuilder::ShaderPackageBuildSettings settings;
        settings.m_rootDirectory = "Shaders";
        settings.m_extensions = { ".hlsl", ".hlsli" };
        settings.m_mounts.push_back({ "", festd::string{ festd::string_view{ prefix.data(), prefix.size() } } });

        AssetBuilder::AssetFile file;
        IO::AssetFileArtifact& artifact = file.m_artifacts.emplace_back();
        artifact.m_productKey = "ShaderPackage/Primary";
        artifact.m_name = "Shaders";
        artifact.m_assetId = packageId;
        artifact.m_assetTypeId = Rtti::GetTypeID<ShaderPackageAsset>();
        artifact.m_buildSettings.Emplace<AssetBuilder::ShaderPackageBuildSettings>(settings);
        ASSERT_TRUE(AssetBuilder::SaveAssetFile(depot / "Assets/Shaders.asset", file));
    }

    // The depot finds the source root above its own directory and tracks the package root as an input directory.
    AssetBuilder::AssetDepot assetDepot;
    AssetBuilder::DepotSettings depotSettings;
    depotSettings.m_depotDirectories.push_back(depot / "Assets");
    depotSettings.m_outputDirectory = depot / "Output";
    ASSERT_TRUE(assetDepot.Scan(depotSettings));

    festd::vector<uint32_t> affected;
    const IO::Path changedShader = IO::NormalizePath(depot / "Shaders/Common/Value.hlsli");
    assetDepot.FindAffectedAssets(festd::span{ &changedShader, 1 }, affected);
    EXPECT_EQ(affected.size(), 1u);

    festd::vector<uint32_t> assets;
    assetDepot.GetAllAssets(assets);
    ASSERT_TRUE(AssetBuilder::BuildDepotAssets(assetDepot, festd::span{ assets.data(), assets.size() }).IsSuccessful());

    IO::ArtifactStore::SetCatalogSource(depot / "Output");
    IO::AssetManager::Init();
    ShaderPackageStreamer streamer;
    IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<ShaderPackageAsset>(), &streamer);
    {
        IO::AssetRequest request = IO::AssetManager::LoadAsset(packageId);
        for (uint32_t iteration = 0; iteration < 5000 && !request.IsCompleted(); ++iteration)
        {
            IO::AssetManager::Tick();
            Threading::Sleep(1);
        }

        ASSERT_EQ(request.GetResult(), IO::AssetLoadResult::kSucceeded) << request.GetError().data();

        const IO::AssetRead<ShaderPackageAsset> package = IO::AssetHandle<ShaderPackageAsset>(request.GetAssetSlot()).Read();
        ASSERT_TRUE(package);
        EXPECT_EQ(package->m_files.size(), 2u);

        Rc sources = Memory::DefaultNew<Core::ShaderSourceCache>();
        const Env::Name header{ Fmt::FixedFormat("{}Common/Value.hlsli", prefix) };
        const festd::expected source = sources->GetSource(header);
        ASSERT_TRUE(source);
        EXPECT_EQ(source.value()->GetSource(), festd::string_view{ "#define VALUE 1\n[numthreads(1, 1, 1)] void main() {}\n" });
        EXPECT_FALSE(sources->GetSource(Env::Name{ Fmt::FixedFormat("{}Notes.txt", prefix) }));

        // The package compiles like a directory would, including its includes.
        const Core::ShaderCompilerArgs args = MakePackageComputeArgs(Env::Name{ Fmt::FixedFormat("{}Probe.cs.hlsl", prefix) });
        EXPECT_TRUE(Core::CreateShaderCompiler()->CompileShader(args).m_codeValid);
    }

    IO::AssetManager::UnregisterStreamer(Rtti::GetTypeID<ShaderPackageAsset>(), &streamer);
    IO::AssetManager::Shutdown();
}
