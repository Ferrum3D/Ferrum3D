#include "FeRenderTarget.h"

namespace FE
{
    FeRenderTarget::FeRenderTarget(DL::ITextureView* rtv, DL::ITextureView* dsv)
    {
        m_RTV = rtv;
        m_DSV = dsv;
    }
} // namespace FE
