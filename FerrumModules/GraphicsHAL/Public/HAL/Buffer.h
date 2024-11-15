#pragma once
#include <HAL/BindFlags.h>
#include <HAL/DeviceObject.h>
#include <HAL/Resource.h>

namespace FE::Graphics::HAL
{
    struct BufferDesc
    {
        uint64_t Size = 0;
        BindFlags Flags = BindFlags::None;

        BufferDesc() = default;

        BufferDesc(uint64_t size, BindFlags bindFlags)
        {
            Size = size;
            Flags = bindFlags;
        }
    };


    class Buffer : public Resource
    {
    public:
        FE_RTTI_Class(Buffer, "2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        virtual ResultCode Init(StringSlice name, const BufferDesc& desc) = 0;

        ~Buffer() override = default;

        virtual void* Map(uint64_t offset, uint64_t size) = 0;
        void* Map(uint64_t offset);

        virtual void Unmap() = 0;

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
