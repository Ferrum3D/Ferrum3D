#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Instance/IInstance.h>

namespace FE::Osmium
{
    class VKInstance : public IInstance
    {
        VkInstance m_Instance;
        VkDebugReportCallbackEXT m_Debug;

        eastl::vector<Rc<IAdapter>> m_Adapters;

    public:
        FE_CLASS_RTTI(VKInstance, "4247535C-3E97-42E7-A869-1DC542AFBF25");

        explicit VKInstance(const InstanceDesc& desc);
        ~VKInstance() override;

        VkInstance GetNativeInstance();

        [[nodiscard]] const eastl::vector<Rc<IAdapter>>& GetAdapters() const override;
    };
} // namespace FE::Osmium
