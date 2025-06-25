#pragma once
#include <Graphics/Core/Meshlet.h>

namespace FE::Graphics::Data
{
    constexpr uint32_t kModelMagic = Math::MakeFourCC('F', 'M', 'D', 0);


    struct ModelHeader final
    {
        uint32_t m_magic;
        uint32_t m_meshCount;
        uint32_t m_lodCount;
    };
} // namespace FE::Graphics::Data
