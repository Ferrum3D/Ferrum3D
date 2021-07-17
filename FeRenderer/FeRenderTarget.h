#pragma once
#include "FeGraphicsDevice.h"
#include "IFeRenderTarget.h"

namespace FE
{
    class FeRenderTarget : public IFeRenderTarget
    {
        DL::RefCntAutoPtr<DL::ITextureView> m_RTV;
        DL::RefCntAutoPtr<DL::ITextureView> m_DSV;

    public:
        FeRenderTarget(DL::ITextureView* rtv, DL::ITextureView* dsv);

        inline DL::ITextureView* GetRTV()
        {
            return m_RTV;
        }

        inline DL::ITextureView* GetDSV()
        {
            return m_DSV;
        }
    };
} // namespace FE
