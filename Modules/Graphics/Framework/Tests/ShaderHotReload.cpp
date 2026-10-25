#include <Core/IO/FileStream.h>
#include <Core/Math/Uuid.h>
#include <Core/Threading/Thread.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/ShaderSourceDirectory.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/PipelineFactory.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

namespace
{
    constexpr festd::string_view kInitialShader = "[numthreads(1, 1, 1)] void main() {}\n";
    constexpr festd::string_view kEditedShader = "// edited\n[numthreads(1, 1, 1)] void main() {}\n";
    constexpr festd::string_view kBrokenShader = "[numthreads(1, 1, 1)] void main() { error }\n";
    constexpr festd::string_view kFixedShader = "// fixed\n[numthreads(1, 1, 1)] void main() {}\n";


    void WriteShaderText(const IO::Path& path, const festd::string_view text)
    {
        ASSERT_EQ(IO::Directory::Create(IO::PathView{ path }.parent_directory()), IO::ResultCode::kSuccess);

        auto stream = IO::FileStream::Open(path, IO::OpenMode::kCreate);
        ASSERT_TRUE(stream);
        ASSERT_EQ(stream.value()->WriteFromBuffer(text.data(), text.size()), text.size());
        stream.value()->Close();
    }


    void RemoveShaderDirectory(const IO::Path& root)
    {
        festd::vector<IO::Path> files;
        festd::vector<IO::Path> directories;
        IO::Directory::TraverseRecursively(root, [&files, &directories](const IO::DirectoryEntry& entry) {
            if (Bit::AnySet(entry.m_attributes, IO::FileAttributeFlags::kDirectory))
                directories.push_back(IO::Path{ entry.m_path });
            else
                files.push_back(IO::Path{ entry.m_path });

            return true;
        });

        for (const IO::Path& file : files)
            IO::File::Delete(file);

        for (auto iter = directories.rbegin(); iter != directories.rend(); ++iter)
            IO::Directory::Delete(*iter);

        IO::Directory::Delete(root);
    }


    //! A shader directory registered under a unique virtual prefix and removed when the test ends.
    struct TestShaderDirectory final
    {
        TestShaderDirectory()
        {
            const festd::fixed_string testId = Fmt::FixedFormat("{}", Uuid::Random());
            m_root = IO::Directory::GetExecutableDirectory() / "ShaderHotReloadTests" / testId;
            m_virtualPrefix = IO::Path{ "ShaderHotReloadTests" } / testId;
            EXPECT_EQ(IO::Directory::Create(m_root), IO::ResultCode::kSuccess);
            Core::AddShaderSourceDirectory(m_root, m_virtualPrefix);
        }

        ~TestShaderDirectory()
        {
            RemoveShaderDirectory(m_root);
        }

        [[nodiscard]] Env::Name GetVirtualPath(const festd::string_view relativePath) const
        {
            return Env::Name{ m_virtualPrefix / relativePath };
        }

        IO::Path m_root;
        IO::Path m_virtualPrefix;
    };


    void PollUntilChanged(Core::ShaderSourceCache& sources, const Env::Name path, festd::vector<Env::Name>& changedPaths)
    {
        for (uint32_t attempt = 0; attempt < 200; ++attempt)
        {
            sources.PollChangedPaths(changedPaths);
            if (festd::find_index(festd::span{ changedPaths.data(), changedPaths.size() }, path) != kInvalidIndex)
                return;

            Threading::Sleep(10);
        }

        ADD_FAILURE() << "No change reported for " << path.c_str();
    }


    //! Drive hot reload like the device does at the end of each frame, until the predicate holds or a timeout expires.
    template<class TPredicate>
    bool UpdateUntil(Vulkan::PipelineFactory& factory, TPredicate&& predicate)
    {
        for (uint32_t attempt = 0; attempt < 1000; ++attempt)
        {
            factory.UpdateShaderHotReload();
            if (predicate())
                return true;

            Threading::Sleep(10);
        }

        return false;
    }


    //! Drive hot reload for a fixed number of updates, long enough for a reload that is expected to change nothing.
    void UpdateFor(Vulkan::PipelineFactory& factory, const uint32_t updateCount)
    {
        for (uint32_t update = 0; update < updateCount; ++update)
        {
            factory.UpdateShaderHotReload();
            Threading::Sleep(10);
        }
    }
} // namespace


TEST(ShaderHotReload, SourceCacheRefreshesWatchedFiles)
{
    const TestShaderDirectory directory;
    const Env::Name header = directory.GetVirtualPath("Common.h");
    const Env::Name unused = directory.GetVirtualPath("Unused.h");
    WriteShaderText(directory.m_root / "Common.h", "#define VALUE 1\n");

    Rc sources = Memory::DefaultNew<Core::ShaderSourceCache>();
    const festd::expected first = sources->GetSource(header);
    ASSERT_TRUE(first);

    // The first poll only starts watching.
    festd::vector<Env::Name> changedPaths;
    sources->PollChangedPaths(changedPaths);

    WriteShaderText(directory.m_root / "Common.h", "#define VALUE 2\n");
    WriteShaderText(directory.m_root / "Unused.h", "#define UNUSED 1\n");
    PollUntilChanged(*sources, header, changedPaths);

    festd::vector<Env::Name> refreshedPaths;
    sources->Refresh(festd::span{ changedPaths.data(), changedPaths.size() }, refreshedPaths);
    ASSERT_EQ(refreshedPaths.size(), 1u);
    EXPECT_EQ(refreshedPaths[0], header);

    const festd::expected second = sources->GetSource(header);
    ASSERT_TRUE(second);
    EXPECT_NE(second.value()->GetContentHash(), first.value()->GetContentHash());
    EXPECT_EQ(second.value()->GetSource(), festd::string_view{ "#define VALUE 2\n" });

    // The previous version stays valid for anything that still holds it.
    EXPECT_EQ(first.value()->GetSource(), festd::string_view{ "#define VALUE 1\n" });

    // Refreshing unchanged contents reports nothing; a file that was never requested is ignored.
    refreshedPaths.clear();
    const Env::Name paths[] = { header, unused };
    sources->Refresh(paths, refreshedPaths);
    EXPECT_TRUE(refreshedPaths.empty());
}


TEST(ShaderHotReload, RebuildsPipelinesInPlaceAndKeepsThemOnCompileErrors)
{
    const TestShaderDirectory directory;
    const IO::Path shaderFile = directory.m_root / "Probe.cs.hlsl";
    WriteShaderText(shaderFile, kInitialShader);

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
        request.m_desc.SetComputeShader(directory.GetVirtualPath("Probe.cs.hlsl"));
        Core::ComputePipeline* pipeline = pipelines->CreateComputePipeline(request);
        pipeline->GetCompletionWaitGroup()->Wait();
        ASSERT_TRUE(pipeline->IsReady());

        // The first update starts watching the registered directories.
        factory.UpdateShaderHotReload();

        const auto getNative = [pipeline] {
            return Vulkan::ImplCast(pipeline)->GetNative();
        };

        const VkPipeline initialNative = getNative();
        const uint32_t initialGeneration = factory.GetShaderReloadGeneration();

        WriteShaderText(shaderFile, kEditedShader);
        ASSERT_TRUE(UpdateUntil(factory, [&] {
            return getNative() != initialNative;
        }));
        EXPECT_TRUE(pipeline->IsReady());
        EXPECT_EQ(factory.GetShaderReloadGeneration(), initialGeneration + 1);

        // A compile error leaves the working pipeline in place.
        const VkPipeline editedNative = getNative();
        const uint32_t editedGeneration = factory.GetShaderReloadGeneration();
        WriteShaderText(shaderFile, kBrokenShader);
        UpdateFor(factory, 150);
        EXPECT_TRUE(pipeline->IsReady());
        EXPECT_EQ(getNative(), editedNative);
        EXPECT_EQ(factory.GetShaderReloadGeneration(), editedGeneration);

        // Fixing the error is picked up like any other change.
        WriteShaderText(shaderFile, kFixedShader);
        ASSERT_TRUE(UpdateUntil(factory, [&] {
            return getNative() != editedNative;
        }));
        EXPECT_TRUE(pipeline->IsReady());
        EXPECT_EQ(factory.GetShaderReloadGeneration(), editedGeneration + 1);
    }

    device->WaitIdle();
}
