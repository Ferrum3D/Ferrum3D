#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool;

    struct Buffer final : public Core::Buffer
    {
        FE_RTTI("CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        ~Buffer() override;

        static Buffer* Create(Core::Device* device, Env::Name name, Core::BufferDesc desc);

        void CommitInternal(ResourcePool* resourcePool, Core::BufferCommitParams params);
        void SwapInternal(BufferInstance*& instance);

        void DecommitMemory() override;

        Core::ResourceMemory GetMemoryStatus() const override;

        void* Map() override;
        void Unmap() override;

        [[nodiscard]] VkBuffer GetNative() const
        {
            FE_AssertDebug(m_instance);
            return m_instance->m_buffer;
        }

        [[nodiscard]] VkBufferView GetView() const
        {
            FE_AssertDebug(m_instance);
            return m_instance->m_view;
        }

    private:
        explicit Buffer(Core::Device* device, Env::Name name, Core::BufferDesc desc);

        void UpdateDebugNames();

        BufferInstance* m_instance = nullptr;
    };


    FE_ENABLE_NATIVE_CAST(Buffer);
} // namespace FE::Graphics::Vulkan
