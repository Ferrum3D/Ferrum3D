#pragma once
#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool;

    struct Buffer final : public Common::Buffer
    {
        FE_RTTI("CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        ~Buffer() override;

        static Buffer* Create(Core::Device* device, Env::Name name, Core::BufferDesc desc);

        void DecommitMemory() override;
        void CommitInternal(ResourcePool* resourcePool, Core::BufferCommitParams params);
        void SwapInternal(BufferInstance*& instance);

        void* Map() override;
        void Unmap() override;

        [[nodiscard]] VkBuffer GetNative() const
        {
            FE_AssertDebug(m_instance);
            return Rtti::AssertCast<BufferInstance*>(m_instance)->m_buffer;
        }

        [[nodiscard]] VkBufferView GetView() const
        {
            FE_AssertDebug(m_instance);
            return Rtti::AssertCast<BufferInstance*>(m_instance)->m_view;
        }

    private:
        explicit Buffer(Core::Device* device, Env::Name name, Core::BufferDesc desc);

        void UpdateDebugNames() const;
    };


    FE_ENABLE_NATIVE_CAST(Buffer);
} // namespace FE::Graphics::Vulkan
