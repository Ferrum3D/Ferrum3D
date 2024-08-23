#include <HAL/Vulkan/CommandQueue.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    Fence::Fence(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode Fence::Init(HAL::FenceState initialState)
    {
        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = initialState == HAL::FenceState::Reset ? 0 : VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(NativeCast(m_pDevice), &fenceCI, VK_NULL_HANDLE, &m_NativeFence);
        return HAL::ResultCode::Success;
    }


    void Fence::SignalOnCPU()
    {
        auto queue = ImplCast(m_pDevice)->GetCommandQueue(HAL::HardwareQueueKindFlags::Graphics);
        queue->SignalFence(this);
    }


    void Fence::WaitOnCPU()
    {
        vkWaitForFences(NativeCast(m_pDevice), 1, &m_NativeFence, false, 10000000000);
    }


    void Fence::Reset()
    {
        vkResetFences(NativeCast(m_pDevice), 1, &m_NativeFence);
    }


    HAL::FenceState Fence::GetState()
    {
        const VkResult status = vkGetFenceStatus(NativeCast(m_pDevice), m_NativeFence);
        return status == VK_SUCCESS ? HAL::FenceState::Signaled : HAL::FenceState::Reset;
    }


    Fence::~Fence()
    {
        vkDestroyFence(NativeCast(m_pDevice), m_NativeFence, nullptr);
    }
} // namespace FE::Graphics::Vulkan
