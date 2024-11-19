#pragma once
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/Resource.h>

namespace FE::Graphics::RHI
{
    struct BufferDesc final
    {
        uint64_t m_size = 0;
        BindFlags m_flags = BindFlags::kNone;

        BufferDesc() = default;

        BufferDesc(uint64_t size, BindFlags bindFlags)
        {
            m_size = size;
            m_flags = bindFlags;
        }
    };


    struct Buffer : public Resource
    {
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
            size = GetDesc().m_size - offset;
        }

        void* map = Map(offset, size);
        memcpy(map, data, size);
        Unmap();
    }
} // namespace FE::Graphics::RHI


FE_MAKE_HASHABLE(FE::Graphics::RHI::BufferDesc, , value.m_size, static_cast<uint32_t>(value.m_flags));
