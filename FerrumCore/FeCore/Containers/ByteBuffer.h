#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Strings/String.h>

namespace FE
{
    //! \brief A buffer that stores contiguous memory as an array of bytes.
    class ByteBuffer final
    {
        UInt8* m_pBegin = nullptr;
        UInt8* m_pEnd = nullptr;

        inline void Allocate(USize size)
        {
            void* ptr = Memory::DefaultAllocate(size);
            m_pBegin = static_cast<UInt8*>(ptr);
            m_pEnd = m_pBegin + size;
        }

        inline void Deallocate()
        {
            Memory::DefaultFree(m_pBegin);
            m_pBegin = nullptr;
            m_pEnd = nullptr;
        }

    public:
        inline ByteBuffer() = default;

        inline ByteBuffer(const ByteBuffer& other)
        {
            Allocate(other.Size());
            memcpy(m_pBegin, other.m_pBegin, other.Size());
        }

        inline ByteBuffer& operator=(const ByteBuffer& other)
        {
            if (&other == this)
            {
                return *this;
            }

            Deallocate();
            Allocate(other.Size());
            memcpy(m_pBegin, other.m_pBegin, other.Size());
            return *this;
        }

        inline ByteBuffer(ByteBuffer&& other) noexcept
        {
            m_pBegin = other.m_pBegin;
            m_pEnd = other.m_pEnd;
            other.m_pBegin = nullptr;
            other.m_pEnd = nullptr;
        }

        inline ByteBuffer& operator=(ByteBuffer&& other) noexcept
        {
            if (&other == this)
                return *this;

            m_pBegin = other.m_pBegin;
            m_pEnd = other.m_pEnd;
            other.m_pBegin = nullptr;
            other.m_pEnd = nullptr;
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
            memcpy(m_pBegin, data.Data(), data.Length());
        }

        //! \brief Create from a list and move it.
        //!
        //! \param [in] data - The list that stores the data.
        explicit ByteBuffer(eastl::vector<UInt8>&& data) noexcept
        {
            m_pBegin = data.begin();
            m_pEnd = data.end();
            data.reset_lose_memory();
        }

        template<class T>
        [[nodiscard]] inline static ByteBuffer MoveList(eastl::vector<T>&& data) noexcept
        {
            ByteBuffer result;
            result.m_pEnd = reinterpret_cast<UInt8*>(data.end());
            result.m_pBegin = reinterpret_cast<UInt8*>(data.DetachData());
            return result;
        }

        template<class T>
        [[nodiscard]] inline static ByteBuffer CopyList(const ArraySlice<T>& data) noexcept
        {
            ByteBuffer result(data.Length());
            memcpy(result.m_pBegin, data.Data(), data.Length());
            return result;
        }

        [[nodiscard]] inline static ByteBuffer CopyString(const StringSlice& data) noexcept
        {
            ByteBuffer result(data.Size());
            memcpy(result.m_pBegin, data.Data(), data.Size());
            return result;
        }

        [[nodiscard]] inline UInt8* Data()
        {
            return m_pBegin;
        }

        [[nodiscard]] inline const UInt8* Data() const
        {
            return m_pBegin;
        }

        [[nodiscard]] inline uint32_t Size() const
        {
            return static_cast<uint32_t>(m_pEnd - m_pBegin);
        }

        inline void Clear()
        {
            Deallocate();
        }
    };
} // namespace FE
