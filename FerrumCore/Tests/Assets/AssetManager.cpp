#include <FeCore/Assets/Asset.h>
#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/Assets/AssetRegistry.h>
#include <FeCore/IO/FileStream.h>
#include <Tests/Assets/TestAssetLoader.h>
#include <Tests/Common/TestCommon.h>

using FE::static_pointer_cast;

TEST(AssetLoader, LoadAsset)
{
    auto loader = FE::MakeShared<FE::Assets::TestAssetLoader>();

    auto deleteCount     = 0;
    auto storage         = static_cast<FE::Assets::TestAssetStorage*>(loader->CreateStorage());
    storage->AddStrongRef();
    storage->DeleteCount = &deleteCount;

    auto file   = FE::MakeShared<FE::IO::FileHandle>();
    auto stream = FE::MakeShared<FE::IO::FileStream>(std::move(file));
    FE_IO_ASSERT(stream->Open(assetPath1, FE::IO::OpenMode::ReadOnly));
    loader->LoadAsset(storage, stream.Get());
    EXPECT_EQ(storage->Data, FE::IO::File::ReadAllText(assetPath1));
    storage->ReleaseStrongRef();
    EXPECT_EQ(deleteCount, 1);

    EXPECT_EQ(loader->GetAssetType(), assetType);
}

TEST(AssetRegistry, AddAsset)
{
    auto registry = FE::MakeShared<FE::Assets::AssetRegistry>();
    registry->AddAsset(assetID1, assetType, assetPath1);
    registry->AddAsset(assetID2, assetType, assetPath2);

    ASSERT_TRUE(registry->HasAsset(assetID1));
    ASSERT_TRUE(registry->HasAsset(assetID2));
    ASSERT_FALSE(registry->HasAsset(FE::Assets::AssetID("F7707EB1-33C8-44A8-A63A-B75D40CD5BAC")));

    EXPECT_EQ(registry->GetAssetFilePath(assetID1), assetPath1);
    EXPECT_EQ(registry->GetAssetFilePath(assetID2), assetPath2);

    EXPECT_EQ(registry->GetAssetType(assetID1), assetType);
    EXPECT_EQ(registry->GetAssetType(assetID2), assetType);
}

TEST(AssetManager, LoadAsset)
{
    auto loader   = FE::MakeShared<FE::Assets::TestAssetLoader>();
    auto registry = FE::MakeShared<FE::Assets::AssetRegistry>();
    registry->AddAsset(assetID1, assetType, assetPath1);
    registry->AddAsset(assetID2, assetType, assetPath2);

    auto provider = FE::MakeShared<FE::Assets::AssetProviderDev>();
    provider->AttachRegistry(registry);

    auto manager = FE::MakeShared<FE::Assets::AssetManager>();
    manager->AttachAssetProvider(static_pointer_cast<FE::Assets::IAssetProvider>(provider));
    manager->RegisterAssetLoader(static_pointer_cast<FE::Assets::IAssetLoader>(loader));

    auto deleteCount = 0;
    {
        auto asset1        = FE::Assets::Asset<FE::Assets::TestAssetStorage>(assetID1);
        auto asset2        = FE::Assets::Asset<FE::Assets::TestAssetStorage>(assetID2);
        auto anotherAsset1 = FE::Assets::Asset<FE::Assets::TestAssetStorage>(assetID1);

        asset1.LoadSync();
        asset1->DeleteCount = &deleteCount;
        asset2.LoadSync();
        asset2->DeleteCount = &deleteCount;
        anotherAsset1.LoadSync();
        anotherAsset1->DeleteCount = &deleteCount;

        EXPECT_EQ(asset1->Data, FE::IO::File::ReadAllText(assetPath1));
        EXPECT_EQ(anotherAsset1->Data, FE::IO::File::ReadAllText(assetPath1));
        //EXPECT_EQ(&anotherAsset1->Data, &asset1->Data);
        EXPECT_EQ(asset2->Data, FE::IO::File::ReadAllText(assetPath2));
    }

    EXPECT_EQ(deleteCount, 3);
}
