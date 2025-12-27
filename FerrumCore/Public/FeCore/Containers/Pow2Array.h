#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    template<class T>
    struct Pow2Array final
    {
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using iterator = T*;
        using const_iterator = const T*;
        using size_type = uint32_t;
        using difference_type = ptrdiff_t;

        Pow2Array(std::pmr::memory_resource* allocator = nullptr);
        ~Pow2Array();

        Pow2Array(const Pow2Array& other);
        Pow2Array(Pow2Array&& other) noexcept;
        Pow2Array& operator=(const Pow2Array& other);
        Pow2Array& operator=(Pow2Array&& other) noexcept;

        void set_allocator(std::pmr::memory_resource* allocator);

        [[nodiscard]] uint32_t size() const;
        [[nodiscard]] uint32_t capacity() const;
        [[nodiscard]] bool empty() const;

        [[nodiscard]] T* data();
        [[nodiscard]] const T* data() const;

        [[nodiscard]] T& operator[](uint32_t index);
        [[nodiscard]] const T& operator[](uint32_t index) const;

        void* push_back_uninitialized();
        void push_back(const T& value);
        void push_back(T&& value);

        void pop_back();

        void clear();
        void shrink_to_fit();
        void reset_loose_memory();

        iterator erase_unsorted(const_iterator it);

        void resize(uint32_t size, const T& value = T());
        void resize_uninitialized(uint32_t size);
        void reserve(uint32_t newCapacity);

        [[nodiscard]] iterator begin();
        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] iterator end();
        [[nodiscard]] const_iterator end() const;

        operator festd::span<T>();
        operator festd::span<const T>() const;

    private:
        Memory::ShortPtr<T> m_data = nullptr;
        Memory::ShortPtr<std::pmr::memory_resource> m_allocator;
        uint32_t m_size : 24;
        uint32_t m_capacityLog2 : 8;
    };


    template<class T>
    Pow2Array<T>::Pow2Array(std::pmr::memory_resource* allocator)
    {
        if (allocator == nullptr)
            allocator = std::pmr::get_default_resource();

        m_allocator = allocator;
        m_size = 0;
        m_capacityLog2 = 0;
    }


    template<class T>
    Pow2Array<T>::~Pow2Array()
    {
        clear();
        shrink_to_fit();
    }


    template<class T>
    Pow2Array<T>::Pow2Array(const Pow2Array& other)
    {
        m_allocator = other.m_allocator;
        m_size = other.m_size;
        m_capacityLog2 = other.m_capacityLog2;
        m_data = Memory::AllocateArray<T>(m_allocator.Get(), capacity());
        festd::uninitialized_copy_n(other.m_data.Get(), m_size, m_data.Get());
    }


    template<class T>
    Pow2Array<T>::Pow2Array(Pow2Array&& other) noexcept
    {
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
    }


    template<class T>
    Pow2Array<T>& Pow2Array<T>::operator=(const Pow2Array& other)
    {
        if (this != &other)
        {
            clear();
            shrink_to_fit();

            m_allocator = other.m_allocator;
            m_size = other.m_size;
            m_capacityLog2 = other.m_capacityLog2;
            m_data = Memory::AllocateArray<T>(m_allocator.Get(), capacity());
            festd::uninitialized_copy_n(other.m_data.Get(), m_size, m_data.Get());
        }

        return *this;
    }


    template<class T>
    Pow2Array<T>& Pow2Array<T>::operator=(Pow2Array&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            shrink_to_fit();

            memcpy(this, &other, sizeof(*this));
            memset(&other, 0, sizeof(*this));
        }

        return *this;
    }


    template<class T>
    void Pow2Array<T>::set_allocator(std::pmr::memory_resource* allocator)
    {
        FE_CoreAssertDebug(capacity() == 0 && m_data == nullptr);
        m_allocator = allocator;
    }


    template<class T>
    uint32_t Pow2Array<T>::size() const
    {
        return m_size;
    }


    template<class T>
    uint32_t Pow2Array<T>::capacity() const
    {
        if (m_capacityLog2 == 0)
            return 0;

        return 1 << (m_capacityLog2 - 1);
    }


    template<class T>
    bool Pow2Array<T>::empty() const
    {
        return m_size == 0;
    }


    template<class T>
    T* Pow2Array<T>::data()
    {
        return m_data.Get();
    }


    template<class T>
    const T* Pow2Array<T>::data() const
    {
        return m_data.Get();
    }


    template<class T>
    T& Pow2Array<T>::operator[](const uint32_t index)
    {
        return m_data[index];
    }


    template<class T>
    const T& Pow2Array<T>::operator[](const uint32_t index) const
    {
        return m_data[index];
    }


    template<class T>
    void* Pow2Array<T>::push_back_uninitialized()
    {
        const uint32_t currentCapacity = capacity();
        if (m_size == currentCapacity)
            reserve(currentCapacity * 2);

        return &m_data[m_size++];
    }


    template<class T>
    void Pow2Array<T>::push_back(const T& value)
    {
        new (push_back_uninitialized()) T(value);
    }


    template<class T>
    void Pow2Array<T>::push_back(T&& value)
    {
        new (push_back_uninitialized()) T(std::move(value));
    }


    template<class T>
    void Pow2Array<T>::pop_back()
    {
        FE_CoreAssertDebug(m_size > 0);
        m_data[--m_size].~T();
    }


    template<class T>
    void Pow2Array<T>::clear()
    {
        for (uint32_t i = 0; i < m_size; i++)
            m_data[i].~T();

        m_size = 0;
    }


    template<class T>
    void Pow2Array<T>::shrink_to_fit()
    {
        const uint32_t newCapacity = Math::CeilPowerOfTwo(m_size);
        const uint32_t currentCapacity = capacity();
        if (currentCapacity > newCapacity)
        {
            if (m_size == 0)
            {
                m_allocator->deallocate(m_data.Get(), currentCapacity * sizeof(T), alignof(T));
                m_data = nullptr;
                m_capacityLog2 = 0;
            }
            else
            {
                const uint32_t newCapacityLog2 = Math::FloorLog2(newCapacity);
                FE_CoreAssertDebug(1 << newCapacityLog2 == newCapacity);

                T* newData = Memory::AllocateArray<T>(m_allocator.Get(), newCapacity);
                festd::uninitialized_copy_n(m_data.Get(), m_size, newData);
                m_allocator->deallocate(m_data.Get(), currentCapacity * sizeof(T), alignof(T));
                m_data = newData;
                m_capacityLog2 = newCapacityLog2 + 1;
            }
        }
    }


    template<class T>
    void Pow2Array<T>::reset_loose_memory()
    {
        std::pmr::memory_resource* allocator = m_allocator.Get();
        memset(this, 0, sizeof(*this));
        m_allocator = allocator;
    }


    template<class T>
    typename Pow2Array<T>::iterator Pow2Array<T>::erase_unsorted(const_iterator it)
    {
        iterator beginIt = m_data;
        iterator endIt = beginIt + m_size - 1;
        FE_CoreAssertDebug(it >= beginIt && it < endIt);

        iterator nonConst = const_cast<iterator>(it);
        *nonConst = std::move(*endIt);

        --m_size;
        endIt->~T();

        return nonConst;
    }


    template<class T>
    void Pow2Array<T>::resize(const uint32_t size, const T& value)
    {
        if (size < m_size)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (uint32_t i = size; i < m_size; i++)
                    m_data[i].~T();
            }

            m_size = size;
        }
        else if (size > m_size)
        {
            const uint32_t prevSize = m_size;
            resize_uninitialized(size);
            festd::uninitialized_fill_n(m_data.Get() + prevSize, size - prevSize, value);
        }
    }


    template<class T>
    void Pow2Array<T>::resize_uninitialized(const uint32_t size)
    {
        if (size > m_size)
            reserve(size);
        m_size = size;
    }


    template<class T>
    void Pow2Array<T>::reserve(uint32_t newCapacity)
    {
        newCapacity = Math::CeilPowerOfTwo(newCapacity);
        const uint32_t currentCapacity = capacity();
        if (newCapacity > currentCapacity)
        {
            const uint32_t newCapacityLog2 = Math::FloorLog2(newCapacity);
            FE_CoreAssertDebug(1 << newCapacityLog2 == newCapacity);

            T* newData = Memory::AllocateArray<T>(m_allocator.Get(), newCapacity);
            festd::uninitialized_copy_n(m_data.Get(), m_size, newData);
            m_allocator->deallocate(m_data.Get(), currentCapacity * sizeof(T), alignof(T));
            m_data = newData;
            m_capacityLog2 = newCapacityLog2 + 1;
        }
    }


    template<class T>
    typename Pow2Array<T>::iterator Pow2Array<T>::begin()
    {
        return m_data.Get();
    }


    template<class T>
    typename Pow2Array<T>::const_iterator Pow2Array<T>::begin() const
    {
        return m_data.Get();
    }


    template<class T>
    typename Pow2Array<T>::iterator Pow2Array<T>::end()
    {
        return m_data.Get() + m_size;
    }


    template<class T>
    typename Pow2Array<T>::const_iterator Pow2Array<T>::end() const
    {
        return m_data.Get() + m_size;
    }


    template<class T>
    Pow2Array<T>::operator festd::span<T>()
    {
        return festd::span<T>(m_data.Get(), m_size);
    }

    template<class T>
    Pow2Array<T>::operator festd::span<const T>() const
    {
        return festd::span<const T>(m_data.Get(), m_size);
    }
} // namespace FE
