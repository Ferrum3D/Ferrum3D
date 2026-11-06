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
            return std::bit_cast<uint64_t>(*this);
        }

        friend bool operator==(const BufferDesc lhs, const BufferDesc rhs)
        {
            return std::bit_cast<uint64_t>(lhs) == std::bit_cast<uint64_t>(rhs);
        }

        friend bool operator!=(const BufferDesc lhs, const BufferDesc rhs)
        {
            return std::bit_cast<uint64_t>(lhs) != std::bit_cast<uint64_t>(rhs);
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


    template<class T>
    struct StructuredBuffer;


    struct Buffer : public Resource
    {
        FE_RTTI("2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        static Rc<Buffer> Create(Device* device, Env::Name name, const BufferDesc& desc);

        template<class T>
        static StructuredBuffer<T> CreateStructured(Device* device, Env::Name name, uint32_t elementCount);

        static Rc<Buffer> CreateByteAddress(Device* device, const Env::Name name, const uint32_t byteSize)
        {
            BufferDesc desc;
            desc.m_format = Format::kUndefined;
            desc.m_size = byteSize;
            return Create(device, name, desc);
        }

        [[nodiscard]] virtual uint64_t GetDeviceAddress() const = 0;

        [[nodiscard]] virtual void* Map() = 0;
        virtual void Unmap() = 0;
        virtual void FlushMappedRange(uint32_t offset, uint32_t byteSize) = 0;
        virtual void InvalidateMappedRange(uint32_t offset, uint32_t byteSize) = 0;

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
            slice.m_size = realSize;
            return Create(m_resource, slice);
        }

        [[nodiscard]] uint64_t GetDeviceAddress() const
        {
            FE_Assert(IsValid());
            return m_resource->GetDeviceAddress() + m_slice.m_offset;
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

    //! Non-owning reference to a buffer of T.
    template<class T>
    struct StructuredBufferPtr final
    {
        Buffer* m_buffer = nullptr;

        StructuredBufferPtr() = default;

        StructuredBufferPtr(std::nullptr_t) {}

        explicit StructuredBufferPtr(Buffer* buffer)
            : m_buffer(buffer)
        {
        }

        [[nodiscard]] operator BufferView() const
        {
            return BufferView(m_buffer);
        }

        [[nodiscard]] Buffer* operator->() const
        {
            return m_buffer;
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_buffer != nullptr;
        }

        friend bool operator==(const StructuredBufferPtr buffer, std::nullptr_t)
        {
            return buffer.m_buffer == nullptr;
        }

        friend bool operator!=(const StructuredBufferPtr buffer, std::nullptr_t)
        {
            return buffer.m_buffer != nullptr;
        }

        //! For data that shaders read in larger elements than they write, e.g. counters that form pairs.
        template<class TOther>
        [[nodiscard]] StructuredBufferPtr<TOther> As() const
        {
            FE_AssertDebug(m_buffer == nullptr || m_buffer->GetDesc().m_size % sizeof(TOther) == 0);
            return StructuredBufferPtr<TOther>(m_buffer);
        }
    };


    //! Owning reference to a buffer of T. FrameGraph::GetSRV and GetUAV keep T, so shader data cannot name the wrong buffer.
    template<class T>
    struct StructuredBuffer final
    {
        Rc<Buffer> m_buffer;

        StructuredBuffer() = default;

        StructuredBuffer(std::nullptr_t) {}

        explicit StructuredBuffer(Rc<Buffer> buffer)
            : m_buffer(std::move(buffer))
        {
        }

        StructuredBuffer(const StructuredBufferPtr<T> buffer)
            : m_buffer(buffer.m_buffer)
        {
        }

        [[nodiscard]] StructuredBufferPtr<T> Get() const
        {
            return StructuredBufferPtr<T>(m_buffer.Get());
        }

        [[nodiscard]] operator StructuredBufferPtr<T>() const
        {
            return Get();
        }

        [[nodiscard]] Buffer* operator->() const
        {
            return m_buffer.Get();
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_buffer != nullptr;
        }

        void Reset()
        {
            m_buffer.Reset();
        }

        friend bool operator==(const StructuredBuffer& buffer, std::nullptr_t)
        {
            return buffer.m_buffer == nullptr;
        }

        friend bool operator!=(const StructuredBuffer& buffer, std::nullptr_t)
        {
            return buffer.m_buffer != nullptr;
        }
    };


    template<class T>
    StructuredBuffer<T> Buffer::CreateStructured(Device* device, const Env::Name name, const uint32_t elementCount)
    {
        BufferDesc desc;
        desc.m_format = Format::kUndefined;
        desc.m_size = sizeof(T) * elementCount;
        return StructuredBuffer<T>(Create(device, name, desc));
    }
} // namespace FE::Graphics::Core


template<>
struct eastl::hash<FE::Graphics::Core::BufferDesc>
{
    size_t operator()(const FE::Graphics::Core::BufferDesc& desc) const noexcept
    {
        return desc.GetHash();
    }
};
