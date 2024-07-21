#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Fence/IFence.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKFence : public IFence
    {
        VkFence m_NativeFence;
        VKDevice* m_Device;

    public:
        FE_CLASS_RTTI(VKFence, "78363647-3381-46F2-97B1-2A1AC8AFC3C1");

        VKFence(VKDevice& dev, FenceState initialState);
        ~VKFence() override;
        void SignalOnCPU() override;
        void WaitOnCPU() override;
        void Reset() override;
        FenceState GetState() override;

        VkFence GetNativeFence();
    };
} // namespace FE::Osmium
