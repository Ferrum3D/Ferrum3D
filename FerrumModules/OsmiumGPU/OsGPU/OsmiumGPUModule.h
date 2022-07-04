#pragma once
#include <FeCore/Framework/ModuleFramework.h>
#include <OsGPU/Instance/IInstance.h>

namespace FE::Osmium
{
    struct OsmiumGPUModuleDesc
    {
        const char* ApplicationName = nullptr;
        GraphicsAPI API             = GraphicsAPI::Vulkan;

        inline OsmiumGPUModuleDesc() = default;

        inline OsmiumGPUModuleDesc(const char* applicationName, GraphicsAPI api)
            : ApplicationName(applicationName)
            , API(api)
        {
        }
    };

    class OsmiumGPUModule : public ModuleFramework<OsmiumGPUModule>
    {
    public:
        ~OsmiumGPUModule() override = default;

        inline static constexpr const char* LibraryPath = "OsGPU";

        virtual void Initialize(const OsmiumGPUModuleDesc& desc) = 0;
        [[nodiscard]] virtual Shared<IInstance> CreateInstance() const = 0;
    };
} // namespace FE::Osmium
