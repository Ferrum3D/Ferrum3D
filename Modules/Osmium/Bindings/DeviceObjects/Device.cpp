#include <Bindings/Shaders/ShaderModule.h>
#include <Bindings/WindowSystem/Window.h>
#include <Bindings/DeviceObjects/RenderPass.h>
#include <GPU/Device/IDevice.h>

namespace FE::GPU
{
    template<class T>
    inline void CopyFromByteBuffer(IByteBuffer* src, List<T>& dst)
    {
        auto size = src->Size() / sizeof(T);
        if (size)
        {
            dst.Resize(size);
            memcpy(dst.Data(), src->Data(), src->Size());
            src->ReleaseStrongRef();
        }
    }

    extern "C"
    {
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
} // namespace FE::GPU
