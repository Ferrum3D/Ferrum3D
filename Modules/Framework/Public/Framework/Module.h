#pragma once
#include <FeCore/Modules/ModuleBase.h>

namespace FE::Framework
{
    struct FrameworkModule : public ModuleBase
    {
        ~FrameworkModule() override = default;

        FE_RTTI_Class(FrameworkModule, "F433B734-4DA2-4F44-80E4-2E9B245EA20B");

        inline static constexpr const char* LibraryPath = "FeFramework";
    };
} // namespace FE::Framework
