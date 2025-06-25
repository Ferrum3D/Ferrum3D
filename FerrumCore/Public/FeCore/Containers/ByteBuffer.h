#pragma once
#include <festd/string.h>
#include <festd/vector.h>

namespace FE
{
    //! @brief A buffer that stores contiguous memory as an array of bytes.
    struct ByteBuffer final
    {
        ByteBuffer()
            : m_allocator(std::pmr::get_default_resource())
        {
        }

        explicit ByteBuffer(std::pmr::memory_resource* allocator)
            : m_allocator(allocator)
        {
        }

        ByteBuffer(const ByteBuffer& other)
        {
            if (&other == this)
                return;

            m_allocator = other.m_allocator;
            Allocate(other.size());

            if (m_begin)
                memcpy(m_begin, other.m_begin, other.size());
        }

        ByteBuffer& operator=(const ByteBuffer& other)
        {
            if (&other == this)
                return *this;

            Deallocate();
            m_allocator = other.m_allocator;

            if (other.m_begin)
            {
                Allocate(other.size());
                memcpy(m_begin, other.m_begin, other.size());
            }

            return *this;
        }

        ByteBuffer(ByteBuffer&& other) noexcept
        {
            if (&other == this)
                return;

            m_begin = other.m_begin;
            m_end = other.m_end;
            m_allocator = other.m_allocator;
            other.m_begin = nullptr;
            other.m_end = nullptr;
            other.m_allocator = nullptr;
        }

        ByteBuffer& operator=(ByteBuffer&& other) noexcept
        {
            if (&other == this)
                return *this;

            Deallocate();
            m_begin = other.m_begin;
            m_end = other.m_end;
            m_allocator = other.m_allocator;
            other.m_begin = nullptr;
            other.m_end = nullptr;
            other.m_allocator = nullptr;
            return *this;
        }

        ~ByteBuffer()
        {
            Deallocate();
        }

        //! @brief Create a ByteBuffer with specified size.
        explicit ByteBuffer(const uint32_t size, std::pmr::memory_resource* allocator = nullptr)
            : m_allocator(allocator ? allocator : std::pmr::get_default_resource())
        {
            Allocate(size);
        }

        //! @brief Copy data from a span.
        explicit ByteBuffer(const festd::span<const std::byte> data, std::pmr::memory_resource* allocator = nullptr)
            : m_allocator(allocator ? allocator : std::pmr::get_default_resource())
        {
            if (data.size() > 0)
            {
                Allocate(data.size());
                memcpy(m_begin, data.data(), data.size());
            }
        }

        template<class T>
        [[nodiscard]] static ByteBuffer MoveFromVector(festd::vector<T>&& data) noexcept
        {
            festd::vector<T> temp = std::move(data);

            ByteBuffer result;
            result.m_begin = reinterpret_cast<std::byte*>(temp.begin());
            result.m_end = reinterpret_cast<std::byte*>(temp.end());
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

        [[nodiscard]] static ByteBuffer CopyFromString(const festd::string_view data) noexcept
        {
            ByteBuffer result(data.size());
            memcpy(result.m_begin, data.data(), data.size());
            return result;
        }

        [[nodiscard]] std::byte* data()
        {
            return m_begin;
        }

        [[nodiscard]] const std::byte* data() const
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

        operator festd::span<const std::byte>() const
        {
            return { m_begin, m_end };
        }

    private:
        std::pmr::memory_resource* m_allocator = nullptr;
        std::byte* m_begin = nullptr;
        std::byte* m_end = nullptr;

        void Allocate(const uint32_t size)
        {
            void* ptr = m_allocator->allocate(size, Memory::kDefaultAlignment);
            m_begin = static_cast<std::byte*>(ptr);
            m_end = m_begin + size;
        }

        void Deallocate()
        {
            if (m_begin)
            {
                m_allocator->deallocate(m_begin, m_end - m_begin);
                m_begin = nullptr;
                m_end = nullptr;
            }
        }
    };
} // namespace FE
