#include <OsGPU/Instance/VKInstance.h>
#include <OsGPU/OsmiumGPUModule.h>

namespace FE::Osmium
{
    class OsmiumGPUModuleImpl : public SharedInterfaceImplBase<OsmiumGPUModule>
    {
        OsmiumGPUModuleDesc m_Desc;

    public:
        FE_CLASS_RTTI(OsmiumGPUModuleImpl, "3DD2CC5D-7629-4A44-A34A-5B84C9A80E95");

        OsmiumGPUModuleImpl();
        ~OsmiumGPUModuleImpl() override = default;

        inline void Initialize(const OsmiumGPUModuleDesc& desc) override
        {
            FrameworkBase::Initialize();
            m_Desc = desc;
        }

        [[nodiscard]] Shared<IInstance> CreateInstance() const override;
    };

    OsmiumGPUModuleImpl::OsmiumGPUModuleImpl()
    {
        SetInfo(ModuleInfo("Osmium.GPU", "Osmium's hardware abstraction layer", "Ferrum3D"));
    }

    Shared<IInstance> OsmiumGPUModuleImpl::CreateInstance() const
    {
        InstanceDesc desc{};
        desc.ApplicationName = m_Desc.ApplicationName;

        switch (m_Desc.API)
        {
        case GraphicsAPI::Vulkan:
            return static_pointer_cast<IInstance>(MakeShared<VKInstance>(desc)).Detach();
        default:
            FE_UNREACHABLE("Invalid value: GraphicsAPI({})", static_cast<Int32>(m_Desc.API));
            break;
        }
        return {};
    }

    extern "C" FE_DLL_EXPORT OsmiumGPUModule* CreateModuleInstance(Env::Internal::IEnvironment* environment)
    {
        Env::AttachEnvironment(*environment);
        return MakeShared<OsmiumGPUModuleImpl>().Detach();
    }
} // namespace FE::Osmium
