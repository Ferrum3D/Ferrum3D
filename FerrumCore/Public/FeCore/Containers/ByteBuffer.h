#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    //! @brief A buffer that stores contiguous memory as an array of bytes.
    struct ByteBuffer final
    {
        ByteBuffer() = default;

        ByteBuffer(const ByteBuffer& other)
        {
            Allocate(other.size());
            memcpy(m_begin, other.m_begin, other.size());
        }

        ByteBuffer& operator=(const ByteBuffer& other)
        {
            if (&other == this)
            {
                return *this;
            }

            Deallocate();
            Allocate(other.size());
            memcpy(m_begin, other.m_begin, other.size());
            return *this;
        }

        ByteBuffer(ByteBuffer&& other) noexcept
        {
            m_begin = other.m_begin;
            m_end = other.m_end;
            other.m_begin = nullptr;
            other.m_end = nullptr;
        }

        ByteBuffer& operator=(ByteBuffer&& other) noexcept
        {
            if (&other == this)
                return *this;

            m_begin = other.m_begin;
            m_end = other.m_end;
            other.m_begin = nullptr;
            other.m_end = nullptr;
            return *this;
        }

        ~ByteBuffer()
        {
            Deallocate();
        }

        //! @brief Create a ByteBuffer with specified size.
        explicit ByteBuffer(uint32_t size)
        {
            Allocate(size);
        }

        //! @brief Copy data from a span.
        explicit ByteBuffer(festd::span<uint8_t> data)
        {
            Allocate(data.size());
            memcpy(m_begin, data.data(), data.size());
        }

        //! @brief Move data from a vector.
        explicit ByteBuffer(festd::vector<uint8_t>&& data) noexcept
        {
            m_begin = data.begin();
            m_end = data.end();
            data.reset_lose_memory();
        }

        template<class T>
        [[nodiscard]] static ByteBuffer MoveFromVector(festd::vector<T>&& data) noexcept
        {
            festd::vector<T> temp = std::move(data);

            ByteBuffer result;
            result.m_begin = reinterpret_cast<uint8_t*>(temp.begin());
            result.m_end = reinterpret_cast<uint8_t*>(temp.end());
            temp.reset_lose_memory();
            return result;
        }

        template<class T>
        [[nodiscard]] static ByteBuffer CopyFromSpan(festd::span<T> data) noexcept
        {
            ByteBuffer result(data.size_bytes());
            memcpy(result.m_begin, data.data(), data.size_bytes());
            return result;
        }

        [[nodiscard]] static ByteBuffer CopyFromString(StringSlice data) noexcept
        {
            ByteBuffer result(data.Size());
            memcpy(result.m_begin, data.Data(), data.Size());
            return result;
        }

        [[nodiscard]] uint8_t* data()
        {
            return m_begin;
        }

        [[nodiscard]] const uint8_t* data() const
        {
            return m_begin;
        }

        [[nodiscard]] uint32_t size() const
        {
            return static_cast<uint32_t>(m_end - m_begin);
        }

        void clear()
        {
            Deallocate();
        }

    private:
        uint8_t* m_begin = nullptr;
        uint8_t* m_end = nullptr;

        void Allocate(uint32_t size)
        {
            void* ptr = Memory::DefaultAllocate(size);
            m_begin = static_cast<uint8_t*>(ptr);
            m_end = m_begin + size;
        }

        void Deallocate()
        {
            Memory::DefaultFree(m_begin);
            m_begin = nullptr;
            m_end = nullptr;
        }
    };
} // namespace FE
