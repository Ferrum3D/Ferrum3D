#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Instance/IInstance.h>

namespace FE::GPU
{
    class VKInstance : public Object<IInstance>
    {
        // Destructors of class members in C++ are called in reverse order
        // so the loader must be declared first to be destructed after the instance
        vk::DynamicLoader m_Loader;
        vk::UniqueInstance m_Instance;
        vk::UniqueDebugReportCallbackEXT m_Debug;

        Vector<Shared<IAdapter>> m_PhysicalDevices;

    public:
        FE_CLASS_RTTI(VKInstance, "4247535C-3E97-42E7-A869-1DC542AFBF25");

        explicit VKInstance(const InstanceDesc& desc);
        ~VKInstance() override;

        vk::Instance& GetNativeInstance();

        Vector<Shared<IAdapter>>& GetAdapters() override;
    };
} // namespace FE::GPU
