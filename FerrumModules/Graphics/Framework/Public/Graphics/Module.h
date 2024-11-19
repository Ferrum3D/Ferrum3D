#pragma once
#include <FeCore/Framework/ModuleBase.h>
#include <Graphics/Assets/ImageAssetStorage.h>
#include <Graphics/Assets/MeshAssetStorage.h>
#include <Graphics/Assets/ShaderAssetStorage.h>

namespace FE::Graphics
{
    struct GraphicsModule : public ModuleBase
    {
        ~GraphicsModule() override = default;

        FE_RTTI_Class(GraphicsModule, "1746E16A-F5EF-4AD0-A91D-541CA8D5F2E8");

        inline static constexpr const char* LibraryPath = "FeGraphics";
    };
} // namespace FE::Graphics
