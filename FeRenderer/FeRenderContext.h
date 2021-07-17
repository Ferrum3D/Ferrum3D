#pragma once
#include "FeGraphicsDevice.h"
#include "IFeRenderContext.h"

namespace FE
{
	class FeRenderContext : public IFeRenderContext
	{
		DL::RefCntAutoPtr<DL::IRenderDevice> m_Device;
		DL::RefCntAutoPtr<DL::IDeviceContext> m_Context;

		struct
		{
			DL::RefCntAutoPtr<DL::ITextureView> RTV;
			DL::RefCntAutoPtr<DL::ITextureView> DSV;
		} m_CurrentRT;

	public:
		FeRenderContext(Diligent::IRenderDevice* dev, Diligent::IDeviceContext* ctx);

		virtual void SetRenderTarget(IFeRenderTarget* renderTarget) override;
		virtual void ClearRenderTarget(float4 color, float depth) override;
	};
}
