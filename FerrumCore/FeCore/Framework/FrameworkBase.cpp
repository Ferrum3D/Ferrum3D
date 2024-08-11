#include <FeCore/Framework/FrameworkBase.h>
#include <FeCore/Modules/EnvironmentPrivate.h>

namespace FE
{
    bool FrameworkBase::IsInitialized()
    {
        return m_IsInitialized;
    }


    void FrameworkBase::GetFrameworkDependencies(eastl::vector<FE::Rc<FE::IFrameworkFactory>>&) {}


    void FrameworkBase::Initialize()
    {
        GetFrameworkDependencies(m_Dependencies);

        for (auto dependency : m_Dependencies)
        {
            dependency->Load();
        }

        DI::ServiceRegistryBuilder builder{ Env::Internal::GetRootServiceRegistry() };
        RegisterServices(builder);
        builder.Build();

        m_IsInitialized = true;
    }


    void FrameworkBase::UnloadDependencies()
    {
        for (auto dependency : m_Dependencies)
        {
            dependency->Unload();
        }
    }
} // namespace FE
