#pragma once
#include <FeCore/Framework/ModuleBase.h>
#include <Graphics/RHI/DeviceFactory.h>

namespace FE::Graphics::RHI
{
    struct GraphicsRHIModule : public ModuleBase
    {
        ~GraphicsRHIModule() override = default;

        FE_RTTI_Class(GraphicsRHIModule, "A47C6079-CFF3-4653-80DA-9146664D1800");

        inline static constexpr const char* LibraryPath = "FeGraphicsRHI";
    };
} // namespace FE::Graphics::RHI
