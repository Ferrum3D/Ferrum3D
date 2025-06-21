#pragma once
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Data
{
    constexpr uint32_t kTextureMagic = Math::MakeFourCC('F', 'T', 'X', 0);


    struct TextureHeader final
    {
        uint32_t m_magic;
        Core::ImageDesc m_desc;
    };


    struct MipChainInfo final
    {
        uint32_t m_mostDetailedMipSlice : 4;
        uint32_t m_mipSliceCount : 4;
        uint32_t m_arraySlice : 12;
        uint32_t m_blockCount : 10;
        uint32_t m_reserved : 2;
    };

    static_assert(sizeof(TextureHeader) == 16);
    static_assert(sizeof(MipChainInfo) == 4);
} // namespace FE::Graphics::Data
