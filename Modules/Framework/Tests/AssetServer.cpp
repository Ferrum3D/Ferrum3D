#include <AssetBuilder/AssetDepot.h>
#include <AssetBuilder/AssetFile.h>
#include <AssetBuilder/AssetPipeline.h>
#include <AssetBuilder/AssetServer.h>
#include <AssetBuilder/BuildState.h>
#include <Core/Base/PlatformInclude.h>
#include <Core/IO/FileStream.h>
#include <Core/Strings/Encoding.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/Thread.h>
#include <EntityTestTypes.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Framework::Tests;

namespace
{
    festd::vector<std::byte> ReadBytes(const IO::Path& path)
    {
        festd::vector<std::byte> bytes;
        auto stream = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
        EXPECT_TRUE(stream);
        if (!stream)
            return bytes;

        bytes.resize(static_cast<uint32_t>(stream.value()->Length()));
        stream.value()->ReadToBuffer(bytes.data(), bytes.size());
        return bytes;
    }


    bool ContainsProduct(const festd::span<const IO::AssetID> products, const IO::AssetID assetId)
    {
        return festd::find_index(products, assetId) != kInvalidIndex;
    }


    //! A depot with a source-less Number asset and an AssetComponent asset that links to it.
    struct AssetServerTest : testing::Test
    {
        void SetUp() override
        {
            m_depot = IO::Path(FE_ENTITY_ASSET_TEST_OUTPUT) / Fmt::Format("Depot/{}", Uuid::Random());
            m_output = m_depot / "Output";
            ASSERT_EQ(IO::Directory::Create(m_depot / "Definitions"), IO::ResultCode::kSuccess);

            m_leaf = WriteDefinition("Definitions/Leaf.asset", Number{ 1 });
            m_root = WriteDefinition("Definitions/Root.asset", AssetComponent{ IO::Link<Number>(m_leaf), {} });

            m_settings.m_depotDirectories.push_back(m_depot / "Definitions");
            m_settings.m_outputDirectory = m_output;
        }

        //! Create or update a definition without building it.
        template<class T>
        IO::AssetID WriteDefinition(const char* name, const T& value)
        {
            const IO::Path path = m_depot / name;
            if (!IO::File::Exists(path))
                EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));

            AssetBuilder::AssetFile file;
            EXPECT_TRUE(AssetBuilder::LoadAssetFile(path, file));
            file.m_artifacts.front().m_buildSettings.Emplace<T>(value);
            EXPECT_TRUE(AssetBuilder::SaveAssetFile(path, file));

            // Reimporting refreshes the dependencies recorded in the definition.
            EXPECT_TRUE(AssetBuilder::ImportAsset({ {}, path, Rtti::GetTypeID<T>() }));
            return file.m_artifacts.front().m_assetId;
        }

        AssetBuilder::AssetServerSettings MakeServerSettings(const bool useChangeJournal) const
        {
            AssetBuilder::AssetServerSettings settings;
            settings.m_depot = m_settings;
            settings.m_useChangeJournal = useChangeJournal;
            settings.m_enableStatusPipe = false;
            return settings;
        }

        IO::Path m_depot;
        IO::Path m_output;
        IO::AssetID m_leaf;
        IO::AssetID m_root;
        AssetBuilder::DepotSettings m_settings;
    };


    //! Poll the server like `ferrum serve` until a build completes or a timeout expires.
    bool PollUntilBuilt(AssetBuilder::AssetServer& server, const uint32_t previousBuildCount)
    {
        for (uint32_t attempt = 0; attempt < 400; ++attempt)
        {
            server.Poll();
            if (server.GetCompletedBuildCount() > previousBuildCount)
                return true;

            Threading::Sleep(10);
        }

        return false;
    }
} // namespace


TEST_F(AssetServerTest, DepotIndexLinksDefinitionsByProductDependencies)
{
    AssetBuilder::AssetDepot depot;
    ASSERT_TRUE(depot.Scan(m_settings));
    ASSERT_EQ(depot.GetAssets().size(), 2u);

    // Definitions are sorted by path, so Leaf comes before Root.
    const AssetBuilder::DepotAsset& leaf = depot.GetAssets()[0];
    const AssetBuilder::DepotAsset& root = depot.GetAssets()[1];
    EXPECT_TRUE(leaf.m_dependencies.empty());
    ASSERT_EQ(root.m_dependencies.size(), 1u);
    EXPECT_EQ(root.m_dependencies[0], 0u);
    ASSERT_EQ(leaf.m_dependents.size(), 1u);
    EXPECT_EQ(leaf.m_dependents[0], 1u);

    festd::vector<uint32_t> affected;
    depot.FindAffectedAssets(festd::span{ &leaf.m_assetFile, 1 }, affected);
    ASSERT_EQ(affected.size(), 1u);
    EXPECT_EQ(affected[0], 0u);

    depot.AddDependents(affected);
    ASSERT_EQ(affected.size(), 2u);
    EXPECT_EQ(affected[1], 1u);

    EXPECT_TRUE(depot.IsDefinitionPath(festd::string_view{ leaf.m_assetFile.data(), leaf.m_assetFile.size() }));
}


TEST_F(AssetServerTest, BuildKeepsResultsOutOfDefinitionsAndSkipsUpToDateProducts)
{
    const festd::vector<std::byte> leafDefinition = ReadBytes(m_depot / "Definitions/Leaf.asset");

    AssetBuilder::AssetDepot depot;
    ASSERT_TRUE(depot.Scan(m_settings));

    festd::vector<uint32_t> assets;
    depot.GetAllAssets(assets);
    const AssetBuilder::DepotBuildResult first =
        AssetBuilder::BuildDepotAssets(depot, festd::span{ assets.data(), assets.size() });
    ASSERT_TRUE(first.IsSuccessful());
    EXPECT_EQ(first.m_succeededCount, 2u);
    EXPECT_TRUE(ContainsProduct(festd::span{ first.m_publishedProducts.data(), first.m_publishedProducts.size() }, m_leaf));
    EXPECT_TRUE(ContainsProduct(festd::span{ first.m_publishedProducts.data(), first.m_publishedProducts.size() }, m_root));

    // Building never writes to the authored definition; the result goes to the output's build state.
    EXPECT_EQ(ReadBytes(m_depot / "Definitions/Leaf.asset"), leafDefinition);
    AssetBuilder::ProductBuildState state;
    ASSERT_TRUE(AssetBuilder::LoadBuildState(m_output, m_leaf, state));
    EXPECT_TRUE(state.m_artifactId.IsValid());

    const AssetBuilder::DepotBuildResult second =
        AssetBuilder::BuildDepotAssets(depot, festd::span{ assets.data(), assets.size() });
    ASSERT_TRUE(second.IsSuccessful());
    EXPECT_TRUE(second.m_publishedProducts.empty());
}


TEST_F(AssetServerTest, FailedDependencySkipsDependents)
{
    AssetBuilder::AssetDepot depot;
    ASSERT_TRUE(depot.Scan(m_settings));

    // A definition that cannot be read anymore fails its build.
    {
        auto stream = IO::FileStream::Open(m_depot / "Definitions/Leaf.asset", IO::OpenMode::kTruncate);
        ASSERT_TRUE(stream);
        constexpr festd::string_view kGarbage = "not a definition";
        stream.value()->WriteFromBuffer(kGarbage.data(), kGarbage.size());
    }

    festd::vector<uint32_t> assets;
    depot.GetAllAssets(assets);
    const AssetBuilder::DepotBuildResult result =
        AssetBuilder::BuildDepotAssets(depot, festd::span{ assets.data(), assets.size() });
    EXPECT_EQ(result.m_failedCount, 1u);
    EXPECT_EQ(result.m_skippedCount, 1u);
    EXPECT_FALSE(result.IsSuccessful());
}


class AssetServerWatchTest
    : public AssetServerTest
    , public testing::WithParamInterface<bool>
{
};


TEST_P(AssetServerWatchTest, RebuildsChangedDefinitionsAndTheirDependents)
{
    AssetBuilder::AssetServer server{ MakeServerSettings(GetParam()) };
    ASSERT_TRUE(server.Start());
    if (GetParam() && !server.IsUsingChangeJournal())
        GTEST_SKIP() << "The volume has no readable change journal";

    EXPECT_EQ(server.GetCompletedBuildCount(), 1u);
    const uint32_t buildCount = server.GetCompletedBuildCount();

    WriteDefinition("Definitions/Leaf.asset", Number{ 2 });
    ASSERT_TRUE(PollUntilBuilt(server, buildCount));

    // The root links to the leaf's artifact, so its build key changes as well.
    const festd::span<const IO::AssetID> published = server.GetLastPublishedProducts();
    EXPECT_TRUE(ContainsProduct(published, m_leaf));
    EXPECT_TRUE(ContainsProduct(published, m_root));
}


TEST_P(AssetServerWatchTest, NewDefinitionsAreIndexedAndBuilt)
{
    AssetBuilder::AssetServer server{ MakeServerSettings(GetParam()) };
    ASSERT_TRUE(server.Start());
    if (GetParam() && !server.IsUsingChangeJournal())
        GTEST_SKIP() << "The volume has no readable change journal";

    const uint32_t buildCount = server.GetCompletedBuildCount();
    const IO::AssetID added = WriteDefinition("Definitions/Nested/Added.asset", Number{ 7 });
    ASSERT_TRUE(PollUntilBuilt(server, buildCount));
    EXPECT_EQ(server.GetDepot().GetAssets().size(), 3u);
    EXPECT_TRUE(ContainsProduct(server.GetLastPublishedProducts(), added));
}

INSTANTIATE_TEST_SUITE_P(AssetServer, AssetServerWatchTest, testing::Values(true, false),
                         [](const testing::TestParamInfo<bool>& info) {
                             return info.param ? "ChangeJournal" : "DirectoryWatcher";
                         });


TEST_F(AssetServerTest, RestartBuildsChangesMadeWhileStopped)
{
    {
        AssetBuilder::AssetServer server{ MakeServerSettings(true) };
        ASSERT_TRUE(server.Start());
        if (!server.IsUsingChangeJournal())
            GTEST_SKIP() << "The volume has no readable change journal";
    }

    WriteDefinition("Definitions/Leaf.asset", Number{ 3 });

    AssetBuilder::AssetServer server{ MakeServerSettings(true) };
    ASSERT_TRUE(server.Start());
    const festd::span<const IO::AssetID> published = server.GetLastPublishedProducts();
    EXPECT_TRUE(ContainsProduct(published, m_leaf));
    EXPECT_TRUE(ContainsProduct(published, m_root));
}


TEST_F(AssetServerTest, StatusPipeReportsBuildsAndRejectsASecondServer)
{
    AssetBuilder::AssetServerSettings settings = MakeServerSettings(false);
    settings.m_enableStatusPipe = true;

    AssetBuilder::AssetServer server{ settings };
    ASSERT_TRUE(server.Start());

    AssetBuilder::AssetServer secondServer{ settings };
    EXPECT_FALSE(secondServer.Start());

    const festd::string pipeName = AssetBuilder::GetAssetServerPipeName(m_output);
    const Str::Utf8ToUtf16 widePipeName{ pipeName.data(), pipeName.size() };
    const HANDLE client = CreateFileW(widePipeName.ToWideString(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    ASSERT_NE(client, INVALID_HANDLE_VALUE);

    const uint32_t buildCount = server.GetCompletedBuildCount();
    WriteDefinition("Definitions/Leaf.asset", Number{ 4 });
    ASSERT_TRUE(PollUntilBuilt(server, buildCount));

    festd::string received;
    char buffer[1024];
    DWORD bytesAvailable = 0;
    while (PeekNamedPipe(client, nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable > 0)
    {
        DWORD bytesRead = 0;
        ASSERT_TRUE(ReadFile(client, buffer, sizeof(buffer), &bytesRead, nullptr));
        received.append(buffer, bytesRead);
    }

    CloseHandle(client);

    const std::string_view receivedView{ received.data(), received.size() };
    EXPECT_NE(receivedView.find("building: 2 assets"), std::string_view::npos) << received.c_str();
    EXPECT_NE(receivedView.find("built: 2 succeeded"), std::string_view::npos) << received.c_str();
    EXPECT_NE(receivedView.find("idle"), std::string_view::npos) << received.c_str();
}
