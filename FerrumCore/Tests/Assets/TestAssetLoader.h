#pragma once
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Strings/String.h>

inline static auto assetType = FE::Assets::AssetType("EFD32327-D5E2-4503-B9CD-364D3B59C492");

namespace FE::Assets
{
    class TestAssetLoader : public IAssetLoader
    {
    public:
        FE_RTTI_Class(TestAssetLoader, "6A78F118-1A66-4543-AF3A-C43AA4E48572");

        ~TestAssetLoader() override = default;

        [[nodiscard]] inline AssetType GetAssetType() const override;
        [[nodiscard]] inline AssetStorage* CreateStorage() override;
        inline void LoadAsset(AssetStorage* storage, IO::IStream* assetStream) override;
        void LoadRawAsset(const eastl::vector<AssetMetadataField>& metadata, AssetStorage* storage,
                          IO::IStream* assetStream) override;
        void SaveAsset(AssetStorage* storage, IO::IStream* assetStream) override;
        eastl::vector<AssetMetadataField> GetAssetMetadataFields() override;
    };

    AssetType TestAssetLoader::GetAssetType() const
    {
        return assetType;
    }

    class TestAssetStorage : public AssetStorage
    {
    protected:
        void Delete() override
        {
            Data.Clear();
            if (DeleteCount)
            {
                ++*DeleteCount;
            }
        }

    public:
        ~TestAssetStorage() override = default;

        String Data;
        int32_t* DeleteCount = nullptr;

        explicit TestAssetStorage(TestAssetLoader* loader)
            : AssetStorage(loader)
        {
        }
    };

    AssetStorage* TestAssetLoader::CreateStorage()
    {
        return new (Memory::DefaultAllocate(sizeof(TestAssetStorage))) TestAssetStorage(this);
    }

    void TestAssetLoader::LoadAsset(AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* testStorage = static_cast<TestAssetStorage*>(storage);
        auto length = assetStream->Length();
        testStorage->Data = String(length, '\0');
        assetStream->ReadToBuffer(testStorage->Data.Data(), length);
    }

    void TestAssetLoader::LoadRawAsset(const eastl::vector<AssetMetadataField>& /* metadata */, AssetStorage* /* storage */,
                                       IO::IStream* /* assetStream */)
    {
    }

    void TestAssetLoader::SaveAsset(AssetStorage* /* storage */, IO::IStream* /* assetStream */) {}

    eastl::vector<AssetMetadataField> TestAssetLoader::GetAssetMetadataFields()
    {
        return {};
    }
} // namespace FE::Assets

inline constexpr auto assetPath1 = "../../FerrumCore/Tests/Assets/TestAsset1.txt";
inline static auto assetID1 = FE::Assets::AssetID("AB008D2B-4287-4E63-BB5B-0433BF966B7E");

inline constexpr auto assetPath2 = "../../FerrumCore/Tests/Assets/TestAsset2.txt";
inline static auto assetID2 = FE::Assets::AssetID("EDA4E058-9342-472F-9BB6-64306F489FD9");
