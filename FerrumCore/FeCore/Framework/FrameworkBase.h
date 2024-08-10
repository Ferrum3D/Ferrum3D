#pragma once
#include <FeCore/Framework/IFramework.h>

namespace FE
{
    class FrameworkBase : public IFramework
    {
        bool m_IsInitialized = false;
        eastl::vector<Rc<IFrameworkFactory>> m_Dependencies;

    protected:
        void GetFrameworkDependencies(eastl::vector<Rc<IFrameworkFactory>>& dependencies) override;
        void Initialize() final;
        void UnloadDependencies() override;

    public:
        FE_RTTI_Class(FrameworkBase, "4B13A66B-5E7E-41BE-A0D0-DD51E995764B");

        ~FrameworkBase() override = default;

        bool IsInitialized() final;
    };
} // namespace FE
