#include "FeTexture.h"

namespace FE
{
    FeTexture::FeTexture(DL::RefCntAutoPtr<DL::ITexture> texture)
    {
        m_Handle = texture;
    }

    DL::ITexture* FeTexture::GetTexture()
    {
        return m_Handle;
    }

    DL::ITextureView* FeTexture::GetDefaultView(DL::TEXTURE_VIEW_TYPE type)
    {
        return m_Handle->GetDefaultView(type);
    }

    uint3 FeTexture::GetSize()
    {
        auto w = m_Handle->GetDesc().Width;
        auto h = m_Handle->GetDesc().Height;
        auto d = m_Handle->GetDesc().Depth;
        return { w, h, d };
    }
} // namespace FE
