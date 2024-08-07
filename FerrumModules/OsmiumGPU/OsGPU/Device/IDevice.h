﻿#pragma once
#include <OsGPU/Common/BaseTypes.h>
#include <OsGPU/Fence/FenceState.h>
#include <OsGPU/Resource/BindFlags.h>

namespace FE::Osmium
{
    class IInstance;
    class IFence;
    class IAdapter;
    class ICommandQueue;
    class ICommandBuffer;
    class ISwapChain;
    class IBuffer;
    class IShaderModule;
    class IRenderPass;
    class IDescriptorHeap;
    class IShaderCompiler;
    class IGraphicsPipeline;
    class IImageView;
    class IFramebuffer;
    class IWindow;
    class IImage;
    class ISampler;
    class ITransientResourceHeap;

    struct SwapChainDesc;
    struct BufferDesc;
    struct ShaderModuleDesc;
    struct RenderPassDesc;
    struct DescriptorHeapDesc;
    struct GraphicsPipelineDesc;
    struct ImageViewDesc;
    struct FramebufferDesc;
    struct WindowDesc;
    struct ImageDesc;
    struct SamplerDesc;
    struct TransientResourceHeapDesc;

    class IDevice : public Memory::RefCountedObjectBase
    {
    public:
        FE_CLASS_RTTI(IDevice, "23D426E6-3322-4CB2-9800-DEBA7C3DEAC0");

        ~IDevice() override = default;

        virtual void WaitIdle() = 0;

        [[nodiscard]] virtual IAdapter& GetAdapter() = 0;
        [[nodiscard]] virtual IInstance& GetInstance() = 0;

        [[nodiscard]] virtual Rc<IFence> CreateFence(FenceState state) = 0;

        [[nodiscard]] virtual Rc<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass) = 0;
        [[nodiscard]] virtual Rc<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass) = 0;

        [[nodiscard]] virtual Rc<ISwapChain> CreateSwapChain(const SwapChainDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IFramebuffer> CreateFramebuffer(const FramebufferDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IWindow> CreateWindow(const WindowDesc& desc) = 0;

        [[nodiscard]] virtual Rc<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IImageView> CreateImageView(const ImageViewDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IImage> CreateImage(const ImageDesc& desc) = 0;
        [[nodiscard]] virtual Rc<ISampler> CreateSampler(const SamplerDesc& desc) = 0;
        [[nodiscard]] virtual Rc<ITransientResourceHeap> CreateTransientResourceHeap(const TransientResourceHeapDesc& desc) = 0;

        [[nodiscard]] virtual Rc<IShaderModule> CreateShaderModule(const ShaderModuleDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IRenderPass> CreateRenderPass(const RenderPassDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IDescriptorHeap> CreateDescriptorHeap(const DescriptorHeapDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IShaderCompiler> CreateShaderCompiler() = 0;
        [[nodiscard]] virtual Rc<IGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    };
} // namespace FE::Osmium
