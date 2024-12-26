#pragma once
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/Resource.h>

namespace FE::Graphics::RHI
{
    struct BufferDesc final
    {
        uint32_t m_size = 0;
        BindFlags m_flags : 16;
        ResourceUsage m_usage : 16;

        BufferDesc()
        {
            m_flags = BindFlags::kNone;
            m_usage = ResourceUsage::kDeviceOnly;
        }

        BufferDesc(uint32_t size, BindFlags bindFlags, ResourceUsage usage)
            : m_size(size)
            , m_flags(bindFlags)
            , m_usage(usage)
        {
        }
    };


    struct Buffer : public Resource
    {
        FE_RTTI_Class(Buffer, "2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        ~Buffer() override = default;

        virtual void* Map(uint32_t offset, uint32_t size) = 0;
        void* Map(uint32_t offset);

        virtual void Unmap() = 0;

        [[nodiscard]] virtual const BufferDesc& GetDesc() const = 0;

        void UpdateData(const void* data, uint32_t offset = 0, uint32_t size = UINT32_MAX);
    };


    inline void* Buffer::Map(uint32_t offset)
    {
        return Map(offset, UINT32_MAX);
    }


    inline void Buffer::UpdateData(const void* data, uint32_t offset, uint32_t size)
    {
        if (size == UINT32_MAX)
        {
            size = GetDesc().m_size - offset;
        }

        void* map = Map(offset, size);
        memcpy(map, data, size);
        Unmap();
    }
} // namespace FE::Graphics::RHI


template<>
struct eastl::hash<FE::Graphics::RHI::BufferDesc>
{
    size_t operator()(const FE::Graphics::RHI::BufferDesc& value) const noexcept
    {
        return FE::festd::bit_cast<size_t>(value);
    }
};
