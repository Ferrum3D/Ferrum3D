#pragma once
#include <GPU/Buffer/IBuffer.h>
#include <GPU/CommandBuffer/ICommandBuffer.h>
#include <GPU/CommandQueue/ICommandQueue.h>
#include <GPU/Descriptors/IDescriptorHeap.h>
#include <GPU/Fence/IFence.h>
#include <GPU/Framebuffer/IFramebuffer.h>
#include <GPU/ImageView/IImageView.h>
#include <GPU/Pipeline/IGraphicsPipeline.h>
#include <GPU/RenderPass/IRenderPass.h>
#include <GPU/Resource/IResource.h>
#include <GPU/Shader/IShaderCompiler.h>
#include <GPU/Shader/IShaderModule.h>
#include <GPU/SwapChain/ISwapChain.h>
#include <GPU/Window/IWindow.h>

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

        virtual void WaitIdle()                                                                                  = 0;
        [[nodiscard]] virtual IAdapter& GetAdapter()                                                             = 0;
        [[nodiscard]] virtual IInstance& GetInstance()                                                           = 0;
        [[nodiscard]] virtual Shared<IFence> CreateFence(FenceState state)                                       = 0;
        [[nodiscard]] virtual Shared<ICommandQueue> GetCommandQueue(CommandQueueClass cmdQueueClass)             = 0;
        [[nodiscard]] virtual Shared<ICommandBuffer> CreateCommandBuffer(CommandQueueClass cmdQueueClass)        = 0;
        [[nodiscard]] virtual Shared<ISwapChain> CreateSwapChain(const SwapChainDesc& desc)                      = 0;
        [[nodiscard]] virtual Shared<IBuffer> CreateBuffer(BindFlags bindFlags, UInt64 size)                     = 0;
        [[nodiscard]] virtual Shared<IShaderModule> CreateShaderModule(const ShaderModuleDesc& desc)             = 0;
        [[nodiscard]] virtual Shared<IRenderPass> CreateRenderPass(const RenderPassDesc& desc)                   = 0;
        [[nodiscard]] virtual Shared<IDescriptorHeap> CreateDescriptorHeap(const DescriptorHeapDesc& desc)       = 0;
        [[nodiscard]] virtual Shared<IShaderCompiler> CreateShaderCompiler()                                     = 0;
        [[nodiscard]] virtual Shared<IGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
        [[nodiscard]] virtual Shared<IImageView> CreateImageView(const ImageViewDesc& desc)                      = 0;
        [[nodiscard]] virtual Shared<IFramebuffer> CreateFramebuffer(const FramebufferDesc& desc)                = 0;
        [[nodiscard]] virtual Shared<IWindow> CreateWindow(const WindowDesc& desc)                               = 0;
        [[nodiscard]] virtual Shared<IImage> CreateImage(const ImageDesc& desc)                                  = 0;
    };
} // namespace FE::GPU