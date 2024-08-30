#pragma once
#include <FeCore/Strings/String.h>

namespace FE
{
    //! @brief A buffer that stores contiguous memory as an array of bytes.
    class ByteBuffer final
    {
        uint8_t* m_pBegin = nullptr;
        uint8_t* m_pEnd = nullptr;

        inline void Allocate(uint32_t size)
        {
            void* ptr = Memory::DefaultAllocate(size);
            m_pBegin = static_cast<uint8_t*>(ptr);
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
            Allocate(other.size());
            memcpy(m_pBegin, other.m_pBegin, other.size());
        }

        inline ByteBuffer& operator=(const ByteBuffer& other)
        {
            if (&other == this)
            {
                return *this;
            }

            Deallocate();
            Allocate(other.size());
            memcpy(m_pBegin, other.m_pBegin, other.size());
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

        //! @brief Create a ByteBuffer with specified size.
        inline explicit ByteBuffer(uint32_t size)
        {
            Allocate(size);
        }

        //! @brief Copy data from a span.
        explicit ByteBuffer(festd::span<uint8_t> data)
        {
            Allocate(data.size());
            memcpy(m_pBegin, data.data(), data.size());
        }

        //! @brief Move data from a vector.
        explicit ByteBuffer(festd::vector<uint8_t>&& data) noexcept
        {
            m_pBegin = data.begin();
            m_pEnd = data.end();
            data.reset_lose_memory();
        }

        template<class T>
        [[nodiscard]] inline static ByteBuffer MoveFromVector(festd::vector<T>&& data) noexcept
        {
            ByteBuffer result;
            result.m_pBegin = reinterpret_cast<uint8_t*>(data.begin());
            result.m_pEnd = reinterpret_cast<uint8_t*>(data.end());
            data.reset_lose_memory();
            return result;
        }

        template<class T>
        [[nodiscard]] inline static ByteBuffer CopyFromSpan(festd::span<T> data) noexcept
        {
            ByteBuffer result(data.size_bytes());
            memcpy(result.m_pBegin, data.data(), data.size_bytes());
            return result;
        }

        [[nodiscard]] inline static ByteBuffer CopyFromString(StringSlice data) noexcept
        {
            ByteBuffer result(data.Size());
            memcpy(result.m_pBegin, data.Data(), data.Size());
            return result;
        }

        [[nodiscard]] inline uint8_t* data()
        {
            return m_pBegin;
        }

        [[nodiscard]] inline const uint8_t* data() const
        {
            return m_pBegin;
        }

        [[nodiscard]] inline uint32_t size() const
        {
            return static_cast<uint32_t>(m_pEnd - m_pBegin);
        }

        inline void clear()
        {
            Deallocate();
        }
    };
} // namespace FE
