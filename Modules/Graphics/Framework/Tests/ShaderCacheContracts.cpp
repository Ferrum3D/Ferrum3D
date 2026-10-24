#include <Core/IO/FileStream.h>
#include <Core/Math/Uuid.h>
#include <Graphics/Core/Common/ShaderCache.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/ShaderSourceDirectory.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Graphics;

namespace
{
    void WriteText(const IO::Path& path, const festd::string_view text)
    {
        ASSERT_EQ(IO::Directory::Create(IO::PathView{ path }.parent_directory()), IO::ResultCode::kSuccess);

        auto stream = IO::FileStream::Open(path, IO::OpenMode::kCreate);
        ASSERT_TRUE(stream);
        ASSERT_EQ(stream.value()->WriteFromBuffer(text.data(), text.size()), text.size());
        stream.value()->Close();
    }


    void RemoveDirectoryTree(const IO::Path& root)
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

        // Children are visited after their parents, so deleting in reverse order removes the deepest directories first.
        for (auto iter = directories.rbegin(); iter != directories.rend(); ++iter)
            IO::Directory::Delete(*iter);

        IO::Directory::Delete(root);
    }


    festd::vector<Core::ShaderCacheDependency> GetDependencies(Core::ShaderSourceCache& sources,
                                                               const festd::span<const Env::Name> paths)
    {
        festd::vector<Core::ShaderCacheDependency> dependencies;
        for (const Env::Name path : paths)
        {
            const festd::expected source = sources.GetSource(path);
            EXPECT_TRUE(source);
            if (!source)
                continue;

            Core::ShaderCacheDependency& dependency = dependencies.push_back();
            dependency.m_path = path;
            dependency.m_contentHash = source.value()->GetContentHash();
        }

        return dependencies;
    }


    bool LoadsByteCode(const Core::ShaderCache& cache, const uint64_t lookupKey, Core::ShaderSourceCache& sources,
                       const festd::span<const std::byte> expected)
    {
        Core::ShaderCompilerResult result;
        if (!cache.TryLoad(lookupKey, sources, nullptr, result))
            return false;

        EXPECT_TRUE(result.m_codeValid);
        EXPECT_EQ(result.m_byteCodeSize, expected.size());
        EXPECT_EQ(result.m_byteCode.size() % sizeof(uint32_t), 0u);
        return result.m_byteCodeSize == expected.size()
            && memcmp(result.m_byteCode.data(), expected.data(), expected.size()) == 0;
    }
} // namespace


TEST(ShaderCache, ReusesResultsOnlyWhileDependenciesMatch)
{
    const festd::fixed_string testId = Fmt::FixedFormat("{}", Uuid::Random());
    const IO::Path root = IO::Directory::GetExecutableDirectory() / "ShaderCacheTests" / testId;
    const IO::Path sourceDirectory = root / "Sources";
    const auto cleanup = festd::defer([&root] {
        RemoveDirectoryTree(root);
    });

    WriteText(sourceDirectory / "Main.cs.hlsl", "#include <Common.h>\n");
    WriteText(sourceDirectory / "Common.h", "#define VALUE 1\n");

    const IO::Path virtualPrefix = IO::Path{ "ShaderCacheTests" } / testId;
    Core::AddShaderSourceDirectory(sourceDirectory, virtualPrefix);

    const Env::Name paths[] = { Env::Name{ virtualPrefix / "Main.cs.hlsl" }, Env::Name{ virtualPrefix / "Common.h" } };
    const Core::ShaderCache cache{ root / "Cache" };

    constexpr std::byte kFirstByteCode[] = { std::byte{ 1 }, std::byte{ 2 }, std::byte{ 3 }, std::byte{ 4 }, std::byte{ 5 } };
    constexpr std::byte kSecondByteCode[] = { std::byte{ 6 }, std::byte{ 7 }, std::byte{ 8 } };
    constexpr uint64_t kLookupKey = 0x1234'5678'9abc'def0;

    Rc firstSources = Memory::DefaultNew<Core::ShaderSourceCache>();
    EXPECT_FALSE(LoadsByteCode(cache, kLookupKey, *firstSources, kFirstByteCode));

    cache.Store(kLookupKey, GetDependencies(*firstSources, paths), kFirstByteCode);
    EXPECT_TRUE(LoadsByteCode(cache, kLookupKey, *firstSources, kFirstByteCode));
    EXPECT_FALSE(LoadsByteCode(cache, kLookupKey + 1, *firstSources, kFirstByteCode));

    // A source cache keeps the file versions it has read, so a changed include is only seen by a new one.
    WriteText(sourceDirectory / "Common.h", "#define VALUE 2\n");
    EXPECT_TRUE(LoadsByteCode(cache, kLookupKey, *firstSources, kFirstByteCode));

    Rc secondSources = Memory::DefaultNew<Core::ShaderSourceCache>();
    EXPECT_FALSE(LoadsByteCode(cache, kLookupKey, *secondSources, kFirstByteCode));

    cache.Store(kLookupKey, GetDependencies(*secondSources, paths), kSecondByteCode);
    EXPECT_TRUE(LoadsByteCode(cache, kLookupKey, *secondSources, kSecondByteCode));

    // Older results stay in the index, so reverting a change does not require recompiling.
    WriteText(sourceDirectory / "Common.h", "#define VALUE 1\n");
    Rc revertedSources = Memory::DefaultNew<Core::ShaderSourceCache>();
    EXPECT_TRUE(LoadsByteCode(cache, kLookupKey, *revertedSources, kFirstByteCode));
}


TEST(ShaderCache, MissingSourcesAreReportedAndRemembered)
{
    Rc sources = Memory::DefaultNew<Core::ShaderSourceCache>();
    const Env::Name path{ "ShaderCacheTests/Missing/Shader.cs.hlsl" };

    const festd::expected first = sources->GetSource(path);
    ASSERT_FALSE(first);
    EXPECT_EQ(first.error(), IO::ResultCode::kNoFileOrDirectory);

    const festd::expected second = sources->GetSource(path);
    ASSERT_FALSE(second);
    EXPECT_EQ(second.error(), IO::ResultCode::kNoFileOrDirectory);
}
