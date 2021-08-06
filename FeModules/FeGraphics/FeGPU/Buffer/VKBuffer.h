#include <FeGPU/Buffer/IBuffer.h>
#include <FeGPU/Memory/VKDeviceMemory.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKBuffer : public IBuffer
    {
        VKDevice* m_Device;
        RefCountPtr<VKDeviceMemory> m_Memory;

    public:
        BufferDesc Desc;
        vk::UniqueBuffer Buffer;

        VKBuffer(VKDevice& dev, const BufferDesc& desc);

        virtual void* Map(uint64_t offset, uint64_t size = static_cast<uint64_t>(-1)) override;
        virtual void Unmap() override;

        virtual void AllocateMemory(MemoryType type) override;
        virtual void BindMemory(const RefCountPtr<IDeviceMemory>& memory, uint64_t offset) override;
    };
} // namespace FE::GPU
