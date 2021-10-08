#include <FrameGraph/Resource/FrameGraphTexture.h>

namespace FE::FG
{
    FrameGraphTexture::FrameGraphTexture(const FrameGraphTextureDesc& desc)
        : m_Desc(desc)
    {
    }

    void FrameGraphTexture::Realize(GPU::IDevice* device)
    {
        GPU::ImageDesc desc{};
        desc.ImageSize     = m_Desc.TextureSize;
        desc.ImageFormat   = m_Desc.TextureFormat;
        desc.Dimension     = m_Desc.Dimension;
        desc.ArraySize     = m_Desc.ArraySize;
        desc.MipLevelCount = m_Desc.MipLevelCount;
        desc.SampleCount   = m_Desc.SampleCount;
        desc.BindFlags     = GPU::ImageBindFlags::ShaderReadWrite; // TODO: determine from usage in the graph
        m_ActualResource   = device->CreateImage(desc);
    }

    void FrameGraphTexture::Dispose()
    {
        m_ActualResource.Reset();
    }
} // namespace FE::FG
