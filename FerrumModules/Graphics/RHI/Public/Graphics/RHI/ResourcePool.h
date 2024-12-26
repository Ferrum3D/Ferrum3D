#pragma once
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/Image.h>

namespace FE::Graphics::RHI
{
    struct ResourcePool : public DeviceObject
    {
        ~ResourcePool() override = default;

        FE_RTTI_Class(ResourcePool, "389492DC-7AE2-4B58-984C-6A1529EDFB41");

        virtual festd::expected<Image*, ResultCode> CreateImage(Env::Name name, const ImageDesc& desc) = 0;
        virtual festd::expected<Buffer*, ResultCode> CreateBuffer(Env::Name name, const BufferDesc& desc) = 0;
    };
} // namespace FE::Graphics::RHI
