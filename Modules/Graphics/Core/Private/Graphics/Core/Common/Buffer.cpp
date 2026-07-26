#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/DeviceFactory.h>

namespace FE::Graphics::Core
{
    static Memory::SpinLockedPool<Common::Buffer> GBufferPool{ "Graphics/Core/BufferPool" };


    Rc<Buffer> Buffer::Create(Device* device, Env::Name name, const BufferDesc& desc)
    {
        return GBufferPool.New(device, name, desc);
    }
} // namespace FE::Graphics::Core


namespace FE::Graphics::Common
{
    Buffer::Buffer(Core::Device* device, const Env::Name name, const Core::BufferDesc desc)
    {
        m_device = device;
        m_name = name;
        m_desc = desc;
        m_type = Core::ResourceType::kBuffer;
        Register();
    }


    Buffer::~Buffer()
    {
        DecommitMemory();
    }


    Core::ResourceMemory Buffer::GetMemoryStatus() const
    {
        if (m_instance == nullptr)
            return Core::ResourceMemory::kNotCommitted;

        return m_instance->m_memoryStatus;
    }


    void Buffer::DecommitMemory()
    {
        if (m_instance == nullptr)
            return;

        for (auto& barrier : m_queueReleaseBarriers)
            FE_Assert(!barrier.has_value());

        FE_Assert(m_instance->m_pool, "Externally created buffers not implemented");
        m_instance->m_pool->DecommitBufferMemory(this);
    }


    void* Buffer::Map()
    {
        return m_instance->Map(m_device);
    }


    void Buffer::Unmap()
    {
        m_instance->Unmap(m_device);
    }


    void Buffer::FlushMappedRange(const uint32_t offset, const uint32_t byteSize)
    {
        m_instance->FlushMappedRange(m_device, offset, byteSize);
    }


    void Buffer::SetState(const SubresourceState state)
    {
        std::unique_lock lk{ m_lock };
        festd::single(m_instance->m_subresourceStates) = state;
    }


    SubresourceState Buffer::GetState() const
    {
        std::unique_lock lk{ m_lock };
        return festd::single(m_instance->m_subresourceStates);
    }


    void Buffer::AddQueueReleaseBarrier(const Core::BufferBarrierDesc& barrier)
    {
        std::unique_lock lk{ m_lock };

        auto& releaseBarrier = m_queueReleaseBarriers[festd::to_underlying(barrier.m_queueAfter)];
        FE_Assert(!releaseBarrier.has_value());
        releaseBarrier = barrier;
    }


    festd::optional<Core::BufferBarrierDesc> Buffer::RetrieveQueueReleaseBarrier(const Core::DeviceQueueType receiverQueue)
    {
        std::unique_lock lk{ m_lock };

        auto& releaseBarrier = m_queueReleaseBarriers[festd::to_underlying(receiverQueue)];
        if (!releaseBarrier.has_value())
            return festd::nullopt;

        auto result = releaseBarrier;
        releaseBarrier.reset();
        return result;
    }


    void Buffer::SwapInstance(ResourceInstance*& instance)
    {
        if (instance != nullptr)
        {
            FE_Assert(m_desc == instance->m_bufferDesc);
            FE_Assert(instance->m_memoryStatus != Core::ResourceMemory::kNotCommitted);
        }

        std::unique_lock lk{ m_lock };

        for (auto& barrier : m_queueReleaseBarriers)
            FE_Assert(!barrier.has_value());

        ResourceInstance* oldInstance = m_instance;
        m_instance = instance;
        instance = oldInstance;

        m_instance->UpdateDebugNames(m_device, m_name);
    }


    void Buffer::DestroyObject()
    {
        Core::GBufferPool.Delete(this);
    }
} // namespace FE::Graphics::Common
