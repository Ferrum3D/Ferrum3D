#include <Bindings/Common.h>
#include <Bindings/DeviceObjects/DescriptorHeap.h>
#include <Bindings/DeviceObjects/Framebuffer.h>
#include <Bindings/DeviceObjects/GraphicsPipeline.h>
#include <Bindings/DeviceObjects/RenderPass.h>
#include <Bindings/Shaders/ShaderModule.h>
#include <Bindings/WindowSystem/Window.h>
#include <OsGPU/Buffer/IBuffer.h>
#include <OsGPU/CommandBuffer/ICommandBuffer.h>
#include <OsGPU/CommandQueue/ICommandQueue.h>
#include <OsGPU/Descriptors/IDescriptorHeap.h>
#include <OsGPU/Descriptors/IDescriptorTable.h>
#include <OsGPU/Device/IDevice.h>
#include <OsGPU/Fence/IFence.h>
#include <OsGPU/Framebuffer/IFramebuffer.h>
#include <OsGPU/Image/IImage.h>
#include <OsGPU/ImageView/IImageView.h>
#include <OsGPU/Sampler/ISampler.h>
#include <OsGPU/Shader/IShaderCompiler.h>
#include <OsGPU/Shader/IShaderModule.h>
#include <OsGPU/SwapChain/ISwapChain.h>
#include <OsGPU/Window/IWindow.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IDevice_WaitIdle(IDevice* self)
        {
            self->WaitIdle();
        }

        FE_DLL_EXPORT IDescriptorHeap* IDevice_CreateDescriptorHeap(IDevice* self, DescriptorHeapDescBinding* desc)
        {
            DescriptorHeapDesc d;
            d.MaxTables = desc->MaxSets;
            CopyFromByteBuffer(desc->Sizes, d.Sizes);
            return self->CreateDescriptorHeap(d).Detach();
        }

        FE_DLL_EXPORT ICommandBuffer* IDevice_CreateCommandBuffer(IDevice* self, CommandQueueClass queueClass)
        {
            return self->CreateCommandBuffer(queueClass).Detach();
        }

        FE_DLL_EXPORT ISampler* IDevice_CreateSampler(IDevice* self, SamplerDesc* desc)
        {
            return self->CreateSampler(*desc).Detach();
        }

        FE_DLL_EXPORT IImage* IDevice_CreateImage(IDevice* self, ImageDesc* desc)
        {
            return self->CreateImage(*desc).Detach();
        }

        FE_DLL_EXPORT IFramebuffer* IDevice_CreateFramebuffer(IDevice* self, FramebufferDescBinding* desc)
        {
            FramebufferDesc d{};

            List<IImageView*> rtv;
            CopyFromByteBuffer(desc->RenderTargetViews, rtv);
            d.RenderTargetViews.Reserve(rtv.Size());
            for (auto& r : rtv)
            {
                d.RenderTargetViews.Push(r);
            }

            d.RenderPass = desc->RenderPass;
            d.Width      = desc->Width;
            d.Height     = desc->Height;

            return self->CreateFramebuffer(d).Detach();
        }

        FE_DLL_EXPORT IFence* IDevice_CreateFence(IDevice* self, FenceState state)
        {
            return self->CreateFence(state).Detach();
        }

        FE_DLL_EXPORT IGraphicsPipeline* IDevice_CreateGraphicsPipeline(IDevice* self, GraphicsPipelineDescBinding* desc)
        {
            GraphicsPipelineDesc d{};
            d.ColorBlend.BlendConstants = Vector4F{ desc->ColorBlend.BlendConstantX, desc->ColorBlend.BlendConstantY,
                                                    desc->ColorBlend.BlendConstantZ, desc->ColorBlend.BlendConstantW };
            CopyFromByteBuffer(desc->ColorBlend.TargetBlendStates, d.ColorBlend.TargetBlendStates);

            d.InputLayout.Topology = desc->InputLayout.Topology;
            CopyFromByteBuffer(desc->InputLayout.Buffers, d.InputLayout.GetBuffers());
            List<InputStreamAttributeDescBinding> attributes;
            CopyFromByteBuffer(desc->InputLayout.Attributes, attributes);
            for (auto& attribute : attributes)
            {
                InputStreamAttributeDesc a;
                a.BufferIndex    = attribute.BufferIndex;
                a.ElementFormat  = attribute.ElementFormat;
                a.Offset         = attribute.Offset;
                a.ShaderSemantic = attribute.ShaderSemantic;
                d.InputLayout.PushAttribute(a);
            }

            d.Rasterization.CullMode             = desc->Rasterization.CullMode;
            d.Rasterization.PolyMode             = desc->Rasterization.PolyMode;
            d.Rasterization.DepthClampEnabled    = desc->Rasterization.DepthClampDepthBiasRasterDiscardEnabled & 4;
            d.Rasterization.DepthBiasEnabled     = desc->Rasterization.DepthClampDepthBiasRasterDiscardEnabled & 2;
            d.Rasterization.RasterDiscardEnabled = desc->Rasterization.DepthClampDepthBiasRasterDiscardEnabled & 1;

            d.DepthStencil.DepthCompareOp    = desc->DepthStencil.DepthCompareOp;
            d.DepthStencil.DepthTestEnabled  = desc->DepthStencil.DepthTestWriteEnabled & 2;
            d.DepthStencil.DepthWriteEnabled = desc->DepthStencil.DepthTestWriteEnabled & 1;

            d.Viewport = desc->Viewport;
            d.Scissor  = desc->Scissor;

            d.RenderPass = desc->RenderPass;

            List<IDescriptorTable*> descriptorTables;
            CopyFromByteBuffer(desc->DescriptorTables, descriptorTables);
            d.DescriptorTables.Reserve(descriptorTables.Size());
            for (auto* descriptorTable : descriptorTables)
            {
                d.DescriptorTables.Push(descriptorTable);
            }

            List<IShaderModule*> shaders;
            CopyFromByteBuffer(desc->Shaders, shaders);
            d.Shaders.Reserve(shaders.Size());
            for (auto* shader : shaders)
            {
                d.Shaders.Push(shader);
            }

            d.SubpassIndex = desc->SubpassIndex;

            return self->CreateGraphicsPipeline(d).Detach();
        }

        FE_DLL_EXPORT IRenderPass* IDevice_CreateRenderPass(IDevice* self, RenderPassDescBinding* desc)
        {
            RenderPassDesc d{};
            CopyFromByteBuffer(desc->Attachments, d.Attachments);
            CopyFromByteBuffer(desc->SubpassDependencies, d.SubpassDependencies);
            List<SubpassDescBinding> subpasses;
            CopyFromByteBuffer(desc->Subpasses, subpasses);
            for (auto& subpassBinding : subpasses)
            {
                auto& subpass = d.Subpasses.Emplace();
                CopyFromByteBuffer(subpassBinding.InputAttachments, subpass.InputAttachments);
                CopyFromByteBuffer(subpassBinding.PreserveAttachments, subpass.PreserveAttachments);
                CopyFromByteBuffer(subpassBinding.RenderTargetAttachments, subpass.RenderTargetAttachments);
                subpass.DepthStencilAttachment = subpassBinding.DepthStencilAttachment;
            }
            return self->CreateRenderPass(d).Detach();
        }

        FE_DLL_EXPORT IShaderModule* IDevice_CreateShaderModule(IDevice* self, ShaderModuleDescBinding* desc)
        {
            ShaderModuleDesc d;
            d.ByteCode     = desc->ByteCode;
            d.ByteCodeSize = desc->ByteCodeSize;
            d.EntryPoint   = desc->EntryPoint;
            d.Stage        = static_cast<ShaderStage>(desc->Stage);
            return self->CreateShaderModule(d).Detach();
        }

        FE_DLL_EXPORT IBuffer* IDevice_CreateBuffer(IDevice* self, Int32 bindFlags, UInt64 size)
        {
            return self->CreateBuffer(static_cast<BindFlags>(bindFlags), size).Detach();
        }

        FE_DLL_EXPORT ISwapChain* IDevice_CreateSwapChain(IDevice* self, SwapChainDesc* desc)
        {
            return self->CreateSwapChain(*desc).Detach();
        }

        FE_DLL_EXPORT IShaderCompiler* IDevice_CreateShaderCompiler(IDevice* self)
        {
            return self->CreateShaderCompiler().Detach();
        }

        FE_DLL_EXPORT IWindow* IDevice_CreateWindow(IDevice* self, WindowDescBinding* desc)
        {
            WindowDesc d;
            d.Width  = desc->Width;
            d.Height = desc->Height;
            d.Title  = desc->Title;
            return self->CreateWindow(d).Detach();
        }

        FE_DLL_EXPORT ICommandQueue* IDevice_GetCommandQueue(IDevice* self, Int32 cmdQueueClass)
        {
            return self->GetCommandQueue(static_cast<CommandQueueClass>(cmdQueueClass)).Detach();
        }

        FE_DLL_EXPORT void IDevice_Destruct(IDevice* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
