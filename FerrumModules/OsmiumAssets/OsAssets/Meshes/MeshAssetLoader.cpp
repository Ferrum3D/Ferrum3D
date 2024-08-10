#include <FeCore/Console/FeLog.h>
#include <FeCore/Utils/BinarySerializer.h>
#include <OsAssets/Meshes/MeshAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>
#include <OsAssets/Meshes/MeshLoaderImpl.h>

namespace FE::Osmium
{
    inline constexpr StringSlice componentFieldName = "vertex-component-";

    Assets::AssetType MeshAssetLoader::AssetType = Assets::AssetType("77ADC20F-B033-4B55-8498-48B59BB92C08");

    Assets::AssetType MeshAssetLoader::GetAssetType() const
    {
        return AssetType;
    }

    Assets::AssetStorage* MeshAssetLoader::CreateStorage()
    {
        return Memory::DefaultNew<MeshAssetStorage>(this);
    }

    void MeshAssetLoader::SaveAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* imageStorage = static_cast<MeshAssetStorage*>(storage);
        BinarySerializer serializer(assetStream);
        serializer.WriteArray(ArraySlice(imageStorage->m_Components));
        serializer.WriteArray(ArraySlice(imageStorage->m_IndexBuffer));
        serializer.WriteArray(ArraySlice(imageStorage->m_VertexBuffer));
    }

    void Osmium::MeshAssetLoader::LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* imageStorage = static_cast<MeshAssetStorage*>(storage);
        BinarySerializer serializer(assetStream);
        serializer.ReadArray(imageStorage->m_Components);
        serializer.ReadArray(imageStorage->m_IndexBuffer);
        serializer.ReadArray(imageStorage->m_VertexBuffer);
    }

    void MeshAssetLoader::LoadRawAsset(const eastl::vector<Assets::AssetMetadataField>& metadata, Assets::AssetStorage* storage,
                                       IO::IStream* assetStream)
    {
        eastl::vector<MeshVertexComponent> components;
        for (auto& field : metadata)
        {
            if (field.GetKey().StartsWith(componentFieldName))
            {
                auto index =
                    field.GetKey().ASCIISubstring(componentFieldName.Size(), field.GetKey().Size()).Parse<uint32_t>().Unwrap();

                if (components.size() <= index)
                {
                    components.resize(index + 1, MeshVertexComponent::None);
                }

#define FE_PARSE_COMPONENT_ENUM(_name)                                                                                           \
    (field.GetValue<Assets::AssetMetadataType::String>().IsEqualTo(#_name, false))                                               \
    {                                                                                                                            \
        components[index] = MeshVertexComponent::_name;                                                                          \
    }
                // clang-format off
                if FE_PARSE_COMPONENT_ENUM(Position3F)
                else if FE_PARSE_COMPONENT_ENUM(Normal3F)
                else if FE_PARSE_COMPONENT_ENUM(Tangent3F)
                else if FE_PARSE_COMPONENT_ENUM(Bitangent3F)
                else if FE_PARSE_COMPONENT_ENUM(TextureCoordinate2F)
                else if FE_PARSE_COMPONENT_ENUM(Color3F)
                else if FE_PARSE_COMPONENT_ENUM(Color4F)
                else if FE_PARSE_COMPONENT_ENUM(Dummy1F)
                else if FE_PARSE_COMPONENT_ENUM(Dummy2F)
                else if FE_PARSE_COMPONENT_ENUM(Dummy3F)
                else if FE_PARSE_COMPONENT_ENUM(Dummy4F)
                    // clang-format on
#undef FE_PARSE_COMPONENT_ENUM
            }
        }

        while (components.back() == MeshVertexComponent::None)
        {
            components.pop_back();
        }

        for (uint32_t i = 0; i < components.size();)
        {
            if (components[i] == MeshVertexComponent::None)
            {
                // TODO: string format argument indices
                FE_LOG_WARNING("Mesh asset metadata: {}{} was not specified, but components with index >= {} where specified",
                               componentFieldName,
                               i,
                               i);

                // It should not be SwapRemoveAt(i), because we need to keep the order anyway
                components.erase(components.begin() + i);
            }
            else
            {
                ++i;
            }
        }

        if (components.empty())
        {
            FE_LOG_ERROR("The must have at least one vertex component");
            return;
        }

        auto* meshStorage = static_cast<MeshAssetStorage*>(storage);
        auto length = assetStream->Length();
        eastl::vector<int8_t> buffer(static_cast<uint32_t>(length), 0);
        assetStream->ReadToBuffer(buffer.data(), length);

        uint32_t vertexCount;
        auto result =
            LoadMeshFromMemory(buffer, components, meshStorage->m_VertexBuffer, meshStorage->m_IndexBuffer, vertexCount);
        meshStorage->m_Components = components;
        FE_ASSERT_MSG(result, "Failed to load a mesh");
    }

    eastl::vector<Assets::AssetMetadataField> MeshAssetLoader::GetAssetMetadataFields()
    {
        char name[componentFieldName.Size() + 2];
        memcpy(name, componentFieldName.Data(), componentFieldName.Size());
        name[componentFieldName.Size() + 1] = 0;
        if (m_MetadataFields.empty())
        {
            for (char i = 0; i < 8; ++i)
            {
                name[componentFieldName.Size()] = static_cast<char>('0' + i);
                m_MetadataFields.push_back(
                    Assets::AssetMetadataField::Create<Assets::AssetMetadataType::String>(name, "", i == 0));
            }
        }

        return m_MetadataFields;
    }
} // namespace FE::Osmium
