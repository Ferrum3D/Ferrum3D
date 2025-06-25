#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/Assets/Base.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Meshlet.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct ModelAsset final : public Asset
    {
        uint32_t m_lodCount = 0;
        uint32_t m_meshCount = 0;
        festd::vector<float> m_lodErrors;
        festd::inline_vector<Core::MeshInfo> m_meshes;
        festd::inline_vector<Core::MeshLodInfo> m_lods;
        festd::vector<Rc<Core::Buffer>> m_geometryBuffers;

        [[nodiscard]] Core::Buffer* GetGeometryBuffer(const uint32_t lodIndex) const
        {
            return m_geometryBuffers[m_lodCount - lodIndex - 1].Get();
        }

        [[nodiscard]] Core::MeshLodInfo GetLodInfo(const uint32_t meshIndex, const uint32_t lodIndex) const
        {
            return m_lods[meshIndex * m_lodCount + (m_lodCount - lodIndex - 1)];
        }

        [[nodiscard]] Core::MeshInfo GetMeshInfo(const uint32_t meshIndex) const
        {
            return m_meshes[meshIndex];
        }
    };


    struct IModelAssetManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IModelAssetManager, "8E721D85-B882-48E9-AD6E-2AC80A52632E");

        virtual ModelAsset* Load(Env::Name assetName) = 0;
    };
} // namespace FE::Graphics
