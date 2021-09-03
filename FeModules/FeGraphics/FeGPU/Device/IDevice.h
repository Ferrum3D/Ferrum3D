#pragma once
#include <FeGPU/Buffer/IBuffer.h>
#include <FeGPU/CommandBuffer/ICommandBuffer.h>
#include <FeGPU/CommandQueue/ICommandQueue.h>
#include <FeGPU/Descriptors/IDescriptorHeap.h>
#include <FeGPU/Fence/IFence.h>
#include <FeGPU/Framebuffer/IFramebuffer.h>
#include <FeGPU/ImageView/IImageView.h>
#include <FeGPU/Pipeline/IGraphicsPipeline.h>
#include <FeGPU/RenderPass/IRenderPass.h>
#include <FeGPU/Resource/IResource.h>
#include <FeGPU/Shader/IShaderCompiler.h>
#include <FeGPU/Shader/IShaderModule.h>
#include <FeGPU/SwapChain/ISwapChain.h>
#include <FeGPU/Window/IWindow.h>

namespace FE::GPU
{
    enum class CommandQueueClass
    {
        Graphics,
        Compute,
        Transfer
    };

    class IInstance;
    class IAdapter;

    class IDevice : public IObject
    {
    public:
        FE_CLASS_RTTI(IDevice, "23D426E6-3322-4CB2-9800-DEBA7C3DEAC0");

        ~IDevice() override = default;

        virtual void WaitIdle()                                                                    = 0;
        virtual IAdapter& GetAdapter()                                                             = 0;
        virtual IInstance& GetInstance()                                                           = 0;
        virtual Shared<IFence> CreateFence(FenceState state)                                       = 0;
        virtual Shared<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass)             = 0;
        virtual Shared<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass)        = 0;
        virtual Shared<ISwapChain> CreateSwapChain(const SwapChainDesc& desc)                      = 0;
        virtual Shared<IBuffer> CreateBuffer(BindFlags bindFlags, UInt64 size)                     = 0;
        virtual Shared<IShaderModule> CreateShaderModule(const ShaderModuleDesc& desc)             = 0;
        virtual Shared<IRenderPass> CreateRenderPass(const RenderPassDesc& desc)                   = 0;
        virtual Shared<IDescriptorHeap> CreateDescriptorHeap(const DescriptorHeapDesc& desc)       = 0;
        virtual Shared<IShaderCompiler> CreateShaderCompiler()                                     = 0;
        virtual Shared<IGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
        virtual Shared<IImageView> CreateImageView(const ImageViewDesc& desc)                      = 0;
        virtual Shared<IFramebuffer> CreateFramebuffer(const FramebufferDesc& desc)                = 0;
        virtual Shared<IWindow> CreateWindow(const WindowDesc& desc)                               = 0;
    };
} // namespace FE::GPU
