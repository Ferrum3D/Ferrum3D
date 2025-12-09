#pragma once
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Core
{
    struct Texture : public Resource
    {
        FE_RTTI("816F7FB8-A3C4-4D22-B8F0-A88D8DB78F47");

        [[nodiscard]] const TextureDesc& GetDesc() const
        {
            return m_desc;
        }

    protected:
        TextureDesc m_desc = {};
    };


    using TextureView = BaseResourceView<Texture, TextureDesc, TextureSubresource>;
} // namespace FE::Graphics::Core
