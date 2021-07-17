#include "FeGraphicsDevice.h"
#include "FeShader.h"
#include "FeTexture.h"
#include <FeCore/Assets/FeAssetManager.h>
#include <FeCore/Console/FeLog.h>

namespace DL = Diligent;

namespace FE
{
    void FeGraphicsDevice::CreateD2D12Backend(DL::Win32NativeWindow window, std::vector<DL::IDeviceContext*>& contexts)
    {
        auto GetEngineFactoryD3D12 = DL::LoadGraphicsEngineD3D12();
        auto* factory              = GetEngineFactoryD3D12();
        m_EngineFactory            = factory;
        FE_ASSERT_MSG(factory->LoadD3D12(), "Couldn't load Direct3D12");

        DL::EngineD3D12CreateInfo createInfo{};
        createInfo.GraphicsAPIVersion = { 11, 0 };
        if (m_ValidationLevel >= 0)
            createInfo.SetValidationLevel((DL::VALIDATION_LEVEL)m_ValidationLevel);

        uint32_t adapterCount = 0;
        factory->EnumerateAdapters(createInfo.GraphicsAPIVersion, adapterCount, nullptr);
        std::vector<DL::GraphicsAdapterInfo> adapters(adapterCount);
        FE_ASSERT_MSG(adapterCount > 0, "No adapters that support Direct3D12 found");
        factory->EnumerateAdapters(createInfo.GraphicsAPIVersion, adapterCount, adapters.data());

        createInfo.Features                                = DL::DeviceFeatures{ DL::DEVICE_FEATURE_STATE_OPTIONAL };
        createInfo.GPUDescriptorHeapDynamicSize[0]         = 32768;
        createInfo.GPUDescriptorHeapSize[1]                = 128;
        createInfo.GPUDescriptorHeapDynamicSize[1]         = 2048 - 128;
        createInfo.DynamicDescriptorAllocationChunkSize[0] = 32;
        createInfo.DynamicDescriptorAllocationChunkSize[1] = 8;
        createInfo.NumDeferredContexts                     = 0;
        createInfo.NumImmediateContexts                    = 0;

        uint32_t displayModeCount = 0;
        factory->EnumerateDisplayModes(createInfo.GraphicsAPIVersion, 0, 0, DL::TEX_FORMAT_RGBA8_UNORM_SRGB, displayModeCount, nullptr);
        m_DisplayModes.resize(displayModeCount);
        factory->EnumerateDisplayModes(
            createInfo.GraphicsAPIVersion, 0, 0, DL::TEX_FORMAT_RGBA8_UNORM_SRGB, displayModeCount, m_DisplayModes.data());

        size_t immediateContextCount = std::max(1u, createInfo.NumImmediateContexts);
        contexts.resize(createInfo.NumDeferredContexts + immediateContextCount);
        factory->CreateDeviceAndContextsD3D12(createInfo, &m_Device, contexts.data());

        FE_ASSERT_MSG(m_Device, "Couldn't create Direct3D12 graphics device");

        factory->CreateSwapChainD3D12(m_Device, contexts[0], DL::SwapChainDesc{}, DL::FullScreenModeDesc{}, window, &m_Swapchain);
        factory->CreateDefaultShaderSourceStreamFactory(nullptr, &m_ShaderSourceFactory);
    }

    std::shared_ptr<IFeShader> FeGraphicsDevice::CreateShader(const FeShaderLoadDesc& desc, const std::string& sourceCode)
    {
        FeShaderDesc d{};
        d.Name                = desc.Name.c_str();
        d.SourceCode          = sourceCode.c_str();
        d.Type                = desc.Type;
        d.Device              = m_Device;
        d.ShaderSourceFactory = m_ShaderSourceFactory;

        return std::make_shared<FeShader>(d);
    }

    std::shared_ptr<IFeTexture> FeGraphicsDevice::CreateTexture(const TextureLoadDesc& desc, RawAsset imageAsset)
    {
#if 0
		// doesn't work
		RefCntAutoPtr<Image> image;
		RefCntAutoPtr<ITexture> texture;
		RefCntAutoPtr<IDataBlob> fileData(MakeNewRCObj<DataBlobImpl>()(imageAsset.GetSize()));

		memcpy_s(fileData->GetDataPtr(), fileData->GetSize(), imageAsset.Read<void>(), imageAsset.GetSize());

		ImageLoadInfo imgLoadInfo;
		switch (desc.Format)
		{
		case Ferrum::FeImageFileFormat::PNG:
			imgLoadInfo.Format = IMAGE_FILE_FORMAT_PNG;
			break;
		case Ferrum::FeImageFileFormat::JPEG:
			imgLoadInfo.Format = IMAGE_FILE_FORMAT_JPEG;
			break;
		case Ferrum::FeImageFileFormat::TIFF:
			imgLoadInfo.Format = IMAGE_FILE_FORMAT_TIFF;
			break;
		default:
			FE_ASSERT_MSG(false, "Unsupported image format");
			break;
		}
		Image::CreateFromDataBlob(fileData, imgLoadInfo, &image);

		TextureLoadInfo info{};
		info.GenerateMips = desc.GenerateMips;
		info.IsSRGB = desc.IsSRGB;
		CreateTextureFromImage(image, info, m_Device, &texture);

		return std::make_shared<FeTexture>(texture);
#endif
        return std::shared_ptr<IFeTexture>(nullptr);
    }

    FeGraphicsDevice::FeGraphicsDevice(DL::Win32NativeWindow window, const FeGraphicsDeviceDesc& desc)
    {
        std::vector<DL::IDeviceContext*> contexts{};

        switch (desc.Backend)
        {
        case FeRenderBackend::Direct3D12:
            CreateD2D12Backend(window, contexts);
            break;
        default:
            LogTrace(LogType::Error, "Render backend isn't implemented");
            return;
        }
    }
} // namespace FE
