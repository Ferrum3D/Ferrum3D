#include <OsGPU/Instance/VKInstance.h>
#include <OsGPU/OsmiumGPUModule.h>

namespace FE::Osmium
{
    class OsmiumGPUModuleImpl : public ServiceLocatorImplBase<OsmiumGPUModule>
    {
        OsmiumGPUModuleDesc m_Desc;

    public:
        FE_CLASS_RTTI(OsmiumGPUModuleImpl, "CB3A80B7-EED3-4FBF-8694-1ED61246234A");

        OsmiumGPUModuleImpl();
        ~OsmiumGPUModuleImpl() override = default;

        inline void Initialize(const OsmiumGPUModuleDesc& desc) override
        {
            FrameworkBase::Initialize();
            m_Desc = desc;
        }

        [[nodiscard]] Rc<IInstance> CreateInstance() const override;
    };

    OsmiumGPUModuleImpl::OsmiumGPUModuleImpl()
    {
        SetInfo(ModuleInfo("Osmium.GPU", "Osmium's hardware abstraction layer", "Ferrum3D"));
    }

    Rc<IInstance> OsmiumGPUModuleImpl::CreateInstance() const
    {
        InstanceDesc desc{};
        desc.ApplicationName = m_Desc.ApplicationName;

        switch (m_Desc.API)
        {
        case GraphicsAPI::Vulkan:
            return static_cast<IInstance*>(Rc<VKInstance>::DefaultNew(desc));
        default:
            FE_UNREACHABLE("Invalid value: GraphicsAPI({})", static_cast<Int32>(m_Desc.API));
            break;
        }
        return {};
    }

    extern "C" FE_DLL_EXPORT OsmiumGPUModule* CreateModuleInstance(Env::Internal::IEnvironment* environment)
    {
        Env::AttachEnvironment(*environment);
        return Rc<OsmiumGPUModuleImpl>::DefaultNew();
    }
} // namespace FE::Osmium
