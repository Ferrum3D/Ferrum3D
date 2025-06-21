#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/RenderTarget.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
    struct ResourcePool : public DeviceObject
    {
        ~ResourcePool() override = default;

        FE_RTTI_Class(ResourcePool, "389492DC-7AE2-4B58-984C-6A1529EDFB41");

        virtual Texture* CreateTexture(Env::Name name, const ImageDesc& desc) = 0;
        virtual RenderTarget* CreateRenderTarget(Env::Name name, const ImageDesc& desc) = 0;
        virtual Buffer* CreateBuffer(Env::Name name, const BufferDesc& desc) = 0;
    };
} // namespace FE::Graphics::Core
