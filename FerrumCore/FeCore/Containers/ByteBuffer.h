#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Containers/List.h>

namespace FE
{
    //! \brief A buffer that stores contiguous memory as an array of bytes.
    class ByteBuffer final
    {
        UInt8* m_Begin = nullptr;
        UInt8* m_End   = nullptr;

        inline void Allocate(USize size)
        {
            void* ptr = GlobalAllocator<HeapAllocator>::Get().Allocate(size, MaximumAlignment, FE_SRCPOS());
            m_Begin   = static_cast<UInt8*>(ptr);
            m_End     = m_Begin + size;
        }

        inline void Deallocate()
        {
            GlobalAllocator<HeapAllocator>::Get().Deallocate(m_Begin, FE_SRCPOS(), Size());
            m_Begin = nullptr;
            m_End   = nullptr;
        }

    public:
        FE_STRUCT_RTTI(ByteBuffer, "96FB77BA-93CF-4321-A41C-1BCB17969B58");

        inline ByteBuffer() = default;

        inline ByteBuffer(const ByteBuffer& other)
        {
            Allocate(other.Size());
            memcpy(m_Begin, other.m_Begin, other.Size());
        }

        inline ByteBuffer& operator=(const ByteBuffer& other)
        {
            if (&other == this)
            {
                return *this;
            }

            Deallocate();
            Allocate(other.Size());
            memcpy(m_Begin, other.m_Begin, other.Size());
            return *this;
        }

        inline ByteBuffer(ByteBuffer&& other) noexcept
        {
            m_Begin       = other.m_Begin;
            m_End         = other.m_End;
            other.m_Begin = nullptr;
            other.m_End   = nullptr;
        }

        inline ByteBuffer& operator=(ByteBuffer&& other) noexcept
        {
            if (&other == this)
            {
                return *this;
            }

            m_Begin       = other.m_Begin;
            m_End         = other.m_End;
            other.m_Begin = nullptr;
            other.m_End   = nullptr;
            return *this;
        }

        inline ~ByteBuffer()
        {
            Deallocate();
        }

        //! \brief Create a ByteBuffer with specified size.
        inline explicit ByteBuffer(USize size)
        {
            Allocate(size);
        }

        //! \brief Create from a list and copy it.
        //!
        //! \param [in] data - The list that stores the data.
        explicit ByteBuffer(const ArraySlice<UInt8>& data)
        {
            Allocate(data.Length());
            memcpy(m_Begin, data.Data(), data.Length());
        }

        //! \brief Create from a list and move it.
        //!
        //! \param [in] data - The list that stores the data.
        explicit ByteBuffer(List<UInt8>&& data) noexcept
        {
            m_End   = data.end();
            m_Begin = data.DetachData();
        }

        [[nodiscard]] inline UInt8* Data()
        {
            return m_Begin;
        }

        [[nodiscard]] inline const UInt8* Data() const
        {
            return m_Begin;
        }

        [[nodiscard]] inline USize Size() const
        {
            return m_End - m_Begin;
        }

        inline void Clear()
        {
            Deallocate();
        }
    };
} // namespace FE
