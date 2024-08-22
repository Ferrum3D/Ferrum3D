#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <HAL/BindFlags.h>
#include <HAL/Common/BaseTypes.h>
#include <HAL/DeviceMemorySlice.h>
#include <HAL/DeviceObject.h>
#include <HAL/MemoryType.h>

namespace FE::Graphics::HAL
{
    struct BufferDesc
    {
        uint64_t Size = 0;
        BindFlags Flags = BindFlags::None;

        inline BufferDesc() = default;

        inline BufferDesc(uint64_t size, BindFlags bindFlags)
        {
            Size = size;
            Flags = bindFlags;
        }
    };

    class Buffer : public DeviceObject
    {
    public:
        FE_RTTI_Class(Buffer, "2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        virtual ResultCode Init(const BufferDesc& desc) = 0;

        ~Buffer() override = default;

        virtual void* Map(uint64_t offset, uint64_t size) = 0;
        void* Map(uint64_t offset);

        virtual void Unmap() = 0;

        virtual void AllocateMemory(MemoryType type) = 0;

        virtual void BindMemory(const DeviceMemorySlice& memory) = 0;

        [[nodiscard]] virtual const BufferDesc& GetDesc() const = 0;

        void UpdateData(const void* data, uint64_t offset = 0, uint64_t size = static_cast<uint64_t>(-1));
    };

    inline void* Buffer::Map(uint64_t offset)
    {
        return Map(offset, static_cast<uint64_t>(-1));
    }

    inline void Buffer::UpdateData(const void* data, uint64_t offset, uint64_t size)
    {
        if (size == static_cast<uint64_t>(-1))
        {
            size = GetDesc().Size - offset;
        }

        void* map = Map(offset, size);
        memcpy(map, data, size);
        Unmap();
    }
} // namespace FE::Graphics::HAL

FE_MAKE_HASHABLE(FE::Graphics::HAL::BufferDesc, , value.Size, static_cast<int>(value.Flags));
