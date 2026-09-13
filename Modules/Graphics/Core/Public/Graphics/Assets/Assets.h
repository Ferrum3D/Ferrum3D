#pragma once
#include <Core/IO/Assets.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Meshlet.h>
#include <Graphics/Core/Texture.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct TextureAsset;

    struct MeshLodAssetInfo final
    {
        uint32_t m_vertexCount = 0;
        uint32_t m_indexCount = 0;
        uint32_t m_meshletCount = 0;
        uint32_t m_primitiveCount = 0;

        FE_RTTI_Reflect("F49E1661-43F4-4A5E-836B-147BE60828ED");
        FE_RTTI_Serialize();
    };


    struct MeshSubmeshAssetInfo final
    {
        festd::inline_vector<MeshLodAssetInfo, 4> m_lods;

        FE_RTTI_Reflect("16E3BAA2-A8B3-4F66-B186-48E7265AECA7");
        FE_RTTI_Serialize();
    };


    //! One mesh asset. Geometry payloads are ordered from the least detailed LOD to the most detailed LOD.
    //!
    //! Payload zero serializes this header. Every following payload stores one LOD and concatenates, for each submesh, packed
    //! vertices, uint32 indices, meshlet headers, and packed triangles in that order.
    struct MeshAsset final
    {
        uint32_t m_vertexStride = 0;
        festd::inline_vector<MeshSubmeshAssetInfo, 4> m_submeshes;
        festd::inline_vector<float, 4> m_lodErrors;

        FE_SKIP_SERIALIZING Rc<Core::Buffer> m_buffer;

        FE_RTTI("44EBC248-FD5E-4CF4-AB4C-09511B78362C");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };


    //! A flattened model with logical references to all imported mesh and texture products.
    struct ModelAsset final
    {
        festd::inline_vector<IO::Link<MeshAsset>, 4> m_meshes;
        festd::inline_vector<IO::Link<TextureAsset>, 4> m_textures;

        // Compatibility state for graphics consumers that have not migrated to streamed mesh payloads yet.
        FE_SKIP_SERIALIZING uint32_t m_lodCount = 0;

        FE_RTTI("2D0926A6-8312-4D3C-8675-F67BB9A62B67");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();

        Core::MeshLodInfo GetLodInfo(uint32_t, uint32_t) const
        {
            return {};
        }

        Core::Buffer* GetGeometryBuffer(uint32_t) const
        {
            return nullptr;
        }
    };


    //! A texture header plus the always-resident mip tail. Mips use least-detailed-first indexing.
    //!
    //! m_mipTailOffsets contains one byte offset for every resident mip. Larger mip N is stored in artifact payload
    //! 1 + (N - m_mipTailOffsets.size()).
    struct TextureAsset final
    {
        static constexpr uint32_t kMaxMipTailByteSize = 64;

        Core::TextureDesc m_desc;
        festd::vector<std::byte> m_mipTailData;
        festd::inline_vector<uint32_t, 4> m_mipTailOffsets;

        FE_SKIP_SERIALIZING Rc<Core::Texture> m_texture;

        FE_RTTI("78A8F995-B51C-42E0-856D-922999074183");
        FE_RTTI_Reflect();
        FE_RTTI_Serialize();
    };
} // namespace FE::Graphics
