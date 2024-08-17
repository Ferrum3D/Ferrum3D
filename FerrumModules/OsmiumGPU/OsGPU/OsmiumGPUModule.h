#pragma once
#include <FeCore/Framework/ModuleBase.h>
#include <OsGPU/Instance/IInstance.h>

namespace FE::Osmium
{
    class OsmiumGPUModule : public ModuleBase
    {
    public:
        ~OsmiumGPUModule() override = default;

        FE_RTTI_Class(OsmiumGPUModule, "A47C6079-CFF3-4653-80DA-9146664D1800");

        inline static constexpr const char* LibraryPath = "OsGPU";
    };
} // namespace FE::Osmium
