#pragma once
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/Format.h>
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


    struct BufferSlice final
    {
        uint32_t m_offset = 0;
        uint32_t m_size = 0;

        static const BufferSlice kInvalid;

        static BufferSlice CreateWhole(const BufferDesc desc)
        {
            return { 0, desc.m_size };
        }
    };

    inline const BufferSlice BufferSlice::kInvalid = { kInvalidIndex, kInvalidIndex };


    struct Buffer : public Resource
    {
        FE_RTTI("2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        [[nodiscard]] virtual void* Map() = 0;
        virtual void Unmap() = 0;

        [[nodiscard]] const BufferDesc& GetDesc() const
        {
            return m_desc;
        }

    protected:
        BufferDesc m_desc;
    };


    struct BufferView final
    {
        Buffer* m_resource = nullptr;
        BufferSlice m_slice = BufferSlice::kInvalid;

        static const BufferView kInvalid;

        BufferView() = default;
        BufferView(Buffer* resource)
            : m_resource(resource)
        {
            if (resource)
                m_slice = BufferSlice::CreateWhole(resource->GetDesc());
        }

        BufferView(Buffer* resource, const BufferSlice slice)
            : m_resource(resource)
            , m_slice(slice)
        {
        }

        [[nodiscard]] BufferView Slice(const uint32_t offset, const uint32_t byteSize = Constants::kMaxU32) const
        {
            uint32_t realSize = byteSize;
            if (realSize == Constants::kMaxU32)
                realSize = m_slice.m_size - offset;

            FE_AssertDebug(realSize <= m_slice.m_size - offset, "BufferView out of range");

            BufferSlice slice = m_slice;
            slice.m_offset += offset;
            slice.m_size = byteSize;
            return Create(m_resource, slice);
        }

        [[nodiscard]] const BufferDesc& GetBaseDesc() const
        {
            return m_resource->GetDesc();
        }

        [[nodiscard]] Env::Name GetName() const
        {
            return m_resource->GetName();
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_resource != nullptr;
        }

        static BufferView Create(Buffer* resource)
        {
            return { resource };
        }

        static BufferView Create(Buffer* resource, const BufferSlice slice)
        {
            return { resource, slice };
        }
    };

    inline const BufferView BufferView::kInvalid = { nullptr, BufferSlice::kInvalid };
} // namespace FE::Graphics::Core


template<>
struct eastl::hash<FE::Graphics::Core::BufferDesc>
{
    size_t operator()(const FE::Graphics::Core::BufferDesc& desc) const noexcept
    {
        return desc.GetHash();
    }
};
