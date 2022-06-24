#pragma once
#include <FeCore/Memory/SharedPtr.h>
#include <OsGPU/Memory/MemoryType.h>

namespace FE::Osmium
{
    class IDeviceMemory;

    struct BufferDesc
    {
        FE_STRUCT_RTTI(BufferDesc, "2932FBE9-01B0-49C0-BDD5-ED0AD1A29F43");
        UInt64 Size;
    };

    class IBuffer : public IObject
    {
    public:
        FE_CLASS_RTTI(IBuffer, "2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        ~IBuffer() override = default;

        virtual void* Map(UInt64 offset, UInt64 size) = 0;
        void* Map(UInt64 offset);

        virtual void Unmap() = 0;

        virtual void AllocateMemory(MemoryType type) = 0;

        virtual void BindMemory(const Shared<IDeviceMemory>& memory, UInt64 offset) = 0;

        [[nodiscard]] virtual const BufferDesc& GetDesc() const = 0;

        inline void UpdateData(const void* data, UInt64 offset = 0, UInt64 size = static_cast<UInt64>(-1));
    };

    inline void* IBuffer::Map(UInt64 offset)
    {
        return Map(offset, static_cast<UInt64>(-1));
    }

    inline void IBuffer::UpdateData(const void* data, UInt64 offset, UInt64 size)
    {
        if (size == static_cast<UInt64>(-1))
        {
            size = GetDesc().Size;
        }

        void* map = Map(offset, size);
        memcpy(map, data, size);
        Unmap();
    }
} // namespace FE::Osmium
