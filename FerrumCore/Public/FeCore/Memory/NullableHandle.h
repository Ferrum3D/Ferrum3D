#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    //! @brief Represents a nullable index, where 0 can be valid. Used mostly for virtual GPU allocations.
    struct NullableHandle final
    {
        //! @brief Get a NULL value, that holds 0xFFFFFFFFFFFFFFFF
        NullableHandle()
            : NullableHandle(static_cast<size_t>(-1))
        {
        }

        //! @brief Get a NULL value, that holds 0xFFFFFFFFFFFFFFFF
        [[nodiscard]] static NullableHandle Null()
        {
            return NullableHandle(static_cast<size_t>(-1));
        }

        //! @brief Get a zero value (not considered NULL!).
        [[nodiscard]] static NullableHandle Zero()
        {
            return NullableHandle(0);
        }

        //! @brief Create a handle from pointer.
        [[nodiscard]] static NullableHandle FromPtr(void* ptr)
        {
            return NullableHandle(reinterpret_cast<size_t>(ptr));
        }

        //! @brief Create a handle from integral offset.
        [[nodiscard]] static NullableHandle FromOffset(size_t offset)
        {
            return NullableHandle(offset);
        }

        //! @brief Convert to pointer. NullableHandle::Null() becomes nullptr.
        [[nodiscard]] void* ToPtr() const
        {
            return IsNull() ? nullptr : reinterpret_cast<void*>(m_handle);
        }

        //! @brief Get underlying integral value.
        [[nodiscard]] size_t ToOffset() const
        {
            return m_handle;
        }

        //! @brief Check if the handle is NULL.
        [[nodiscard]] bool IsNull() const
        {
            return m_handle == static_cast<size_t>(-1);
        }

        //! @brief Check if the handle is not NULL.
        [[nodiscard]] bool IsValid() const
        {
            return m_handle != static_cast<size_t>(-1);
        }

        [[nodiscard]] explicit operator bool() const // NOLINT(google-explicit-constructor)
        {
            return IsValid();
        }

        //! @brief Reset the value to NULL.
        void Reset()
        {
            m_handle = static_cast<size_t>(-1);
        }

        friend bool operator==(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_handle == rhs.m_handle;
        }

        friend bool operator!=(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_handle != rhs.m_handle;
        }

        friend bool operator<(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_handle < rhs.m_handle;
        }

        friend bool operator>(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_handle > rhs.m_handle;
        }

        friend bool operator<=(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_handle <= rhs.m_handle;
        }

        friend bool operator>=(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_handle >= rhs.m_handle;
        }

        friend NullableHandle operator+(const NullableHandle& handle, size_t offset)
        {
            return NullableHandle(handle.m_handle + offset);
        }

        friend NullableHandle operator-(const NullableHandle& handle, size_t offset)
        {
            return NullableHandle(handle.m_handle - offset);
        }

        NullableHandle& operator+=(size_t offset)
        {
            m_handle += offset;
            return *this;
        }

        NullableHandle& operator-=(size_t offset)
        {
            m_handle -= offset;
            return *this;
        }

    private:
        size_t m_handle;

        explicit NullableHandle(size_t handle)
            : m_handle(handle)
        {
        }
    };

    template<>
    inline NullableHandle AlignUp<NullableHandle, size_t>(NullableHandle x, size_t align)
    {
        return NullableHandle::FromOffset((x.ToOffset() + (align - 1u)) & ~(align - 1u));
    }
} // namespace FE

template<>
struct eastl::hash<FE::NullableHandle>
{
    size_t operator()(const FE::NullableHandle& handle) const noexcept
    {
        return eastl::hash<size_t>{}(handle.ToOffset());
    }
};
