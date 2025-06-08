#pragma once
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Core
{
    struct RenderTarget : public Resource
    {
        FE_RTTI_Class(RenderTarget, "2B9DDC1E-4BBE-4C3E-8817-AAAC369F1E54");

        virtual const ImageDesc& GetDesc() const = 0;
    };
} // namespace FE::Graphics::Core
