#pragma once
#include <vector>
#include "FeRenderAPI.h"
#include "FeRenderInternal.h"
#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Platforms/Win32/interface/Win32NativeWindow.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#include "IFeGraphicsDevice.h"

namespace Ferrum
{
	class FeGraphicsDevice : public IFeGraphicsDevice
	{
		Diligent::RefCntAutoPtr<Diligent::IEngineFactory> m_EngineFactory;
		Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_Device;
		Diligent::RefCntAutoPtr<Diligent::ISwapChain> m_Swapchain;
		std::vector<Diligent::DisplayModeAttribs> m_DisplayModes;
		Diligent::GraphicsAdapterInfo m_AdapterInfo;

#if FE_DEBUG
		int m_ValidationLevel = 1;
#else
		int m_ValidationLevel = 0;
#endif

	public:
		FeGraphicsDevice(Diligent::Win32NativeWindow window, const FeGraphicsDeviceDesc& desc);
		void CreateD2D12Backend(Diligent::Win32NativeWindow window, std::vector<Diligent::IDeviceContext*>& contexts);
	};
}
