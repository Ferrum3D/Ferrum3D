#include "FeGraphicsDevice.h"
#include <FerrumCore/FeLog.h>

using namespace Diligent;

namespace Ferrum
{
	void FeGraphicsDevice::CreateD2D12Backend(Win32NativeWindow window, std::vector<IDeviceContext*>& contexts) {
		auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
		auto* factory = GetEngineFactoryD3D12();
		m_EngineFactory = factory;
		FE_ASSERT_MSG(factory->LoadD3D12(), "Couldn't load Direct3D12");

		EngineD3D12CreateInfo createInfo{};
		createInfo.GraphicsAPIVersion = { 11, 0 };
		if (m_ValidationLevel >= 0)
			createInfo.SetValidationLevel((VALIDATION_LEVEL)m_ValidationLevel);

		uint32_t adapterCount = 0;
		factory->EnumerateAdapters(createInfo.GraphicsAPIVersion, adapterCount, nullptr);
		std::vector<GraphicsAdapterInfo> adapters(adapterCount);
		FE_ASSERT_MSG(adapterCount > 0, "No adapters that support Direct3D12 found");
		factory->EnumerateAdapters(createInfo.GraphicsAPIVersion, adapterCount, adapters.data());

		createInfo.Features = DeviceFeatures{ DEVICE_FEATURE_STATE_OPTIONAL };
		createInfo.GPUDescriptorHeapDynamicSize[0] = 32768;
		createInfo.GPUDescriptorHeapSize[1] = 128;
		createInfo.GPUDescriptorHeapDynamicSize[1] = 2048 - 128;
		createInfo.DynamicDescriptorAllocationChunkSize[0] = 32;
		createInfo.DynamicDescriptorAllocationChunkSize[1] = 8;
		createInfo.NumDeferredContexts = 0;
		createInfo.NumImmediateContexts = 0;
		
		uint32_t displayModeCount = 0;
		factory->EnumerateDisplayModes(createInfo.GraphicsAPIVersion, 0, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, displayModeCount, nullptr);
		m_DisplayModes.resize(displayModeCount);
		factory->EnumerateDisplayModes(createInfo.GraphicsAPIVersion, 0, 0, TEX_FORMAT_RGBA8_UNORM_SRGB, displayModeCount, m_DisplayModes.data());

		uint32_t immediateContextCount = std::max(1u, createInfo.NumImmediateContexts);
		contexts.resize(createInfo.NumDeferredContexts + immediateContextCount);
		factory->CreateDeviceAndContextsD3D12(createInfo, &m_Device, contexts.data());

		FE_ASSERT_MSG(m_Device, "Couldn't create Direct3D12 graphics device");

		factory->CreateSwapChainD3D12(m_Device, contexts[0], SwapChainDesc{}, FullScreenModeDesc{}, window, &m_Swapchain);
	}

	FeGraphicsDevice::FeGraphicsDevice(Win32NativeWindow window, const FeGraphicsDeviceDesc& desc) {
		std::vector<IDeviceContext*> contexts{};

		switch (desc.Backend)
		{
		case FeRenderBackend::Direct3D12:
			CreateD2D12Backend(window, contexts);
			break;
		default:
			FeLog(FeLogType::Error, "Render backend isn't implemented");
			return;
		}
	}
}
