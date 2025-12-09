#pragma once
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Core
{
    struct BufferDesc final
    {
        uint32_t m_size = 0;
        Format m_format = Format::kUndefined;

        [[nodiscard]] uint64_t GetHash() const
        {
            return festd::bit_cast<uint64_t>(*this);
        }

        friend bool operator==(const BufferDesc lhs, const BufferDesc rhs)
        {
            return festd::bit_cast<uint64_t>(lhs) == festd::bit_cast<uint64_t>(rhs);
        }

        friend bool operator!=(const BufferDesc lhs, const BufferDesc rhs)
        {
            return festd::bit_cast<uint64_t>(lhs) != festd::bit_cast<uint64_t>(rhs);
        }
    };


    struct BufferSubresource final
    {
        uint32_t m_offset = 0;
        uint32_t m_size = 0;

        static const BufferSubresource kInvalid;

        static BufferSubresource CreateWhole(const BufferDesc desc)
        {
            return { 0, desc.m_size };
        }
    };

    inline const BufferSubresource BufferSubresource::kInvalid = { kInvalidIndex, kInvalidIndex };


    struct Buffer : public Resource
    {
        FE_RTTI("2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        virtual void* Map() = 0;
        virtual void Unmap() = 0;

        [[nodiscard]] const BufferDesc& GetDesc() const
        {
            return m_desc;
        }

    protected:
        BufferDesc m_desc;
    };


    using BufferView = BaseResourceView<Buffer, BufferDesc, BufferSubresource>;
} // namespace FE::Graphics::Core


template<>
struct eastl::hash<FE::Graphics::Core::BufferDesc>
{
    size_t operator()(const FE::Graphics::Core::BufferDesc& desc) const noexcept
    {
        return desc.GetHash();
    }
};
