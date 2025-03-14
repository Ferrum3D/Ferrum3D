#pragma once
#include <FeCore/Modules/ModuleBase.h>
#include <Graphics/Core/DeviceFactory.h>

namespace FE::Graphics::Core
{
    struct GraphicsCoreModule : public ModuleBase
    {
        ~GraphicsCoreModule() override = default;

        FE_RTTI_Class(GraphicsCoreModule, "A47C6079-CFF3-4653-80DA-9146664D1800");

        inline static constexpr const char* LibraryPath = "FeGraphicsCore";
    };
} // namespace FE::Graphics::Core
