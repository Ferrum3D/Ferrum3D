#include <FeCore/Framework/FrameworkBase.h>

namespace FE
{
    bool FrameworkBase::IsInitialized()
    {
        return m_IsInitialized;
    }

    void FrameworkBase::GetFrameworkDependencies([[maybe_unused]] eastl::vector<FE::Rc<FE::IFrameworkFactory>>& dependencies) {}

    void FrameworkBase::Initialize()
    {
        GetFrameworkDependencies(m_Dependencies);

        for (auto dependency : m_Dependencies)
        {
            dependency->Load();
        }

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
