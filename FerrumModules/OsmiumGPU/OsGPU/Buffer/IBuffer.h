#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <OsGPU/Memory/DeviceMemorySlice.h>
#include <OsGPU/Memory/MemoryType.h>
#include <OsGPU/Resource/BindFlags.h>

namespace FE::Osmium
{
    struct BufferDesc
    {
        FE_RTTI_Base(BufferDesc, "2932FBE9-01B0-49C0-BDD5-ED0AD1A29F43");
        uint64_t Size = 0;
        BindFlags Flags = BindFlags::None;

        inline BufferDesc() = default;

        inline BufferDesc(uint64_t size, BindFlags bindFlags)
        {
            Size = size;
            Flags = bindFlags;
        }
    };

    class IBuffer : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IBuffer, "2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        ~IBuffer() override = default;

        virtual void* Map(uint64_t offset, uint64_t size) = 0;
        void* Map(uint64_t offset);

        virtual void Unmap() = 0;

        virtual void AllocateMemory(MemoryType type) = 0;

        virtual void BindMemory(const DeviceMemorySlice& memory) = 0;

        [[nodiscard]] virtual const BufferDesc& GetDesc() const = 0;

        inline void UpdateData(const void* data, uint64_t offset = 0, uint64_t size = static_cast<uint64_t>(-1));
    };

    inline void* IBuffer::Map(uint64_t offset)
    {
        return Map(offset, static_cast<uint64_t>(-1));
    }

    inline void IBuffer::UpdateData(const void* data, uint64_t offset, uint64_t size)
    {
        if (size == static_cast<uint64_t>(-1))
        {
            size = GetDesc().Size;
        }

        void* map = Map(offset, size);
        memcpy(map, data, size);
        Unmap();
    }
} // namespace FE::Osmium

FE_MAKE_HASHABLE(FE::Osmium::BufferDesc, , value.Size, static_cast<int>(value.Flags));
