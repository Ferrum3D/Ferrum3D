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

        BufferDesc(const uint32_t size, const BindFlags bindFlags, const ResourceUsage usage)
            : m_size(size)
            , m_flags(bindFlags)
            , m_usage(usage)
        {
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return festd::bit_cast<uint64_t>(*this);
        }
    };


    struct Buffer : public Resource
    {
        FE_RTTI_Class(Buffer, "2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        virtual void* Map() = 0;
        virtual void Unmap() = 0;

        [[nodiscard]] virtual const BufferDesc& GetDesc() const = 0;
        void UpdateData(const void* data, uint32_t offset = 0, uint32_t size = UINT32_MAX);
    };


    inline void Buffer::UpdateData(const void* data, uint32_t offset, uint32_t size)
    {
        if (size == UINT32_MAX)
        {
            size = GetDesc().m_size - offset;
        }

        void* map = static_cast<uint8_t*>(Map()) + offset;
        memcpy(map, data, size);
        Unmap();
    }
} // namespace FE::Graphics::RHI


template<>
struct eastl::hash<FE::Graphics::RHI::BufferDesc>
{
    size_t operator()(const FE::Graphics::RHI::BufferDesc& desc) const noexcept
    {
        return desc.GetHash();
    }
};
