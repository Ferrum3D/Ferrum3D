#pragma once
// clang-format off
#include <vector>
#include <atomic>
// These two headers must be included before Diligent Engine
#include "FeRenderAPI.h"
#include "FeRenderInternal.h"
// RefCntAutoPtr must be included before all other Diligent Engine headers
#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Platforms/Win32/interface/Win32NativeWindow.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#include <DiligentCore/Common/interface/DataBlobImpl.hpp>
#include <DiligentTools/TextureLoader/interface/Image.h>
#include <DiligentTools/TextureLoader/interface/TextureUtilities.h>
#include "IFeGraphicsDevice.h"
#include <FeCore/Assets/AssetManager.h>
// clang-format on

namespace DL = Diligent;

namespace FE
{
    class FeGraphicsDevice : public IFeGraphicsDevice
    {
        DL::RefCntAutoPtr<DL::IEngineFactory> m_EngineFactory;
        DL::RefCntAutoPtr<DL::IRenderDevice> m_Device;
        DL::RefCntAutoPtr<DL::ISwapChain> m_Swapchain;
        DL::RefCntAutoPtr<DL::IShaderSourceInputStreamFactory> m_ShaderSourceFactory;
        std::vector<DL::DisplayModeAttribs> m_DisplayModes;
        DL::GraphicsAdapterInfo m_AdapterInfo;

#if FE_DEBUG
        int m_ValidationLevel = 1;
#else
        int m_ValidationLevel = 0;
#endif

    public:
        FeGraphicsDevice(DL::Win32NativeWindow window, const FeGraphicsDeviceDesc& desc);
        void CreateD2D12Backend(DL::Win32NativeWindow window, std::vector<DL::IDeviceContext*>& contexts);
        virtual std::shared_ptr<IFeShader> CreateShader(const FeShaderLoadDesc& desc, const std::string& sourceCode) override;
        virtual std::shared_ptr<IFeTexture> CreateTexture(const TextureLoadDesc& desc, RawAsset imageAsset) override;
    };
} // namespace FE
