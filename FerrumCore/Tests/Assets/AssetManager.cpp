#include <FeCore/Assets/Asset.h>
#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/Assets/AssetRegistry.h>
#include <FeCore/IO/FileStream.h>
#include <FeCore/Modules/Environment.h>
#include <Tests/Assets/TestAssetLoader.h>
#include <Tests/Common/TestCommon.h>

using namespace FE;

TEST(AssetLoader, LoadAsset)
{
    Rc loader = Rc<Assets::TestAssetLoader>::DefaultNew();

    auto deleteCount = 0;
    auto storage = static_cast<Assets::TestAssetStorage*>(loader->CreateStorage());
    storage->AddStrongRef();
    storage->DeleteCount = &deleteCount;

    Rc file = Rc<IO::FileHandle>::DefaultNew();
    Rc stream = Rc<IO::FileStream>::DefaultNew(std::move(file));
    FE_IO_ASSERT(stream->Open(assetPath1, IO::OpenMode::kReadOnly));
    loader->LoadAsset(storage, stream.Get());
    EXPECT_EQ(storage->Data, IO::File::ReadAllText(assetPath1));
    storage->ReleaseStrongRef();
    EXPECT_EQ(deleteCount, 1);

    EXPECT_EQ(loader->GetAssetType(), assetType);
}

TEST(AssetRegistry, AddAsset)
{
    Rc registry = Rc<Assets::AssetRegistry>::DefaultNew();
    registry->AddAsset(assetID1, assetType, assetPath1);
    registry->AddAsset(assetID2, assetType, assetPath2);

    ASSERT_TRUE(registry->HasAsset(assetID1));
    ASSERT_TRUE(registry->HasAsset(assetID2));
    ASSERT_FALSE(registry->HasAsset(Assets::AssetID("F7707EB1-33C8-44A8-A63A-B75D40CD5BAC")));

    EXPECT_EQ(registry->GetAssetFilePath(assetID1), assetPath1);
    EXPECT_EQ(registry->GetAssetFilePath(assetID2), assetPath2);

    EXPECT_EQ(registry->GetAssetType(assetID1), assetType);
    EXPECT_EQ(registry->GetAssetType(assetID2), assetType);
}

TEST(AssetManager, LoadAsset)
{
    Rc loader = Rc<Assets::TestAssetLoader>::DefaultNew();
    Rc registry = Rc<Assets::AssetRegistry>::DefaultNew();
    registry->AddAsset(assetID1, assetType, assetPath1);
    registry->AddAsset(assetID2, assetType, assetPath2);

    Rc provider = Rc<Assets::AssetProviderDev>::DefaultNew();
    provider->AttachRegistry(registry);

    Rc manager = Rc<Assets::AssetManager>::DefaultNew();
    manager->AttachAssetProvider(static_pointer_cast<Assets::IAssetProvider>(provider));
    manager->RegisterAssetLoader(static_pointer_cast<Assets::IAssetLoader>(loader));

    auto deleteCount = 0;
    {
        auto asset1 = Assets::Asset<Assets::TestAssetStorage>(assetID1);
        auto asset2 = Assets::Asset<Assets::TestAssetStorage>(assetID2);
        auto anotherAsset1 = Assets::Asset<Assets::TestAssetStorage>(assetID1);

        asset1.LoadSync(manager.Get());
        asset1->DeleteCount = &deleteCount;
        asset2.LoadSync(manager.Get());
        asset2->DeleteCount = &deleteCount;
        anotherAsset1.LoadSync(manager.Get());
        anotherAsset1->DeleteCount = &deleteCount;

        EXPECT_EQ(asset1->Data, IO::File::ReadAllText(assetPath1));
        EXPECT_EQ(anotherAsset1->Data, IO::File::ReadAllText(assetPath1));
        //EXPECT_EQ(&anotherAsset1->Data, &asset1->Data);
        EXPECT_EQ(asset2->Data, IO::File::ReadAllText(assetPath2));
    }

    EXPECT_EQ(deleteCount, 3);
}
