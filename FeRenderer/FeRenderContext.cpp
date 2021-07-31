#include "FeRenderContext.h"
#include "FeRenderTarget.h"

namespace FE
{
    FeRenderContext::FeRenderContext(DL::IRenderDevice* dev, DL::IDeviceContext* ctx)
    {
        m_Device  = dev;
        m_Context = ctx;
    }

    void FeRenderContext::SetRenderTarget(IFeRenderTarget* renderTarget)
    {
        FeRenderTarget* rt = (FeRenderTarget*)renderTarget;
        m_CurrentRT.RTV    = rt->GetRTV();
        m_CurrentRT.DSV    = rt->GetDSV();
        m_Context->SetRenderTargets(1, &m_CurrentRT.RTV, m_CurrentRT.DSV, DL::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    void FeRenderContext::ClearRenderTarget(float4 color, float depth)
    {
        if (m_CurrentRT.RTV)
            m_Context->ClearRenderTarget(m_CurrentRT.RTV, color.Data(), DL::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        if (m_CurrentRT.DSV)
            m_Context->ClearDepthStencil(m_CurrentRT.DSV, DL::CLEAR_DEPTH_FLAG, depth, 0, DL::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
} // namespace FE
