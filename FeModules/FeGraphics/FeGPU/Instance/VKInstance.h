#pragma once
#include <FeGPU/Instance/IInstance.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKInstance : public IInstance
    {
        // Destructors of class members in C++ are called in reverse order
        // so the loader must be declared first to be destructed after the instance
        vk::DynamicLoader m_Loader;
        vk::UniqueInstance m_Instance;
        vk::UniqueDebugReportCallbackEXT m_Debug;

        Vector<RefCountPtr<IAdapter>> m_PhysicalDevices;

    public:
        VKInstance(const InstanceDesc& desc);
        vk::Instance& GetNativeInstance();

        virtual Vector<RefCountPtr<IAdapter>>& GetAdapters() override;
    };
}
