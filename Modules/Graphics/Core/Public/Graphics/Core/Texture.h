#pragma once
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Core
{
    struct Texture : public Resource
    {
        FE_RTTI_Class(Texture, "816F7FB8-A3C4-4D22-B8F0-A88D8DB78F47");

        const TextureDesc& GetDesc() const
        {
            return m_desc;
        }

    protected:
        TextureDesc m_desc = {};
    };
} // namespace FE::Graphics::Core
