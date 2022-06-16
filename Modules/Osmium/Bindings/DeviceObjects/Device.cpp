#include <Bindings/WindowSystem/Window.h>
#include <GPU/Device/IDevice.h>

namespace FE::GPU
{
    extern "C"
    {
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
