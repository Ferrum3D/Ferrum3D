#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    //! \brief Represents a nullable index, where 0 can be valid. Used mostly for virtual GPU allocations.
    class NullableHandle final
    {
        USize m_Handle;

        inline explicit NullableHandle(USize handle)
            : m_Handle(handle)
        {
        }

    public:
        //! \brief Get a NULL value, that holds 0xFFFFFFFFFFFFFFFF
        inline NullableHandle()
            : NullableHandle(static_cast<USize>(-1))
        {
        }

        //! \brief Get a NULL value, that holds 0xFFFFFFFFFFFFFFFF
        [[nodiscard]] inline static NullableHandle Null()
        {
            return NullableHandle(static_cast<USize>(-1));
        }

        //! \brief Get a zero value (not considered NULL!).
        [[nodiscard]] inline static NullableHandle Zero()
        {
            return NullableHandle(0);
        }

        //! \brief Create a handle from pointer.
        [[nodiscard]] inline static NullableHandle FromPtr(void* ptr)
        {
            return NullableHandle(reinterpret_cast<USize>(ptr));
        }

        //! \brief Create a handle from integral offset.
        [[nodiscard]] inline static NullableHandle FromOffset(USize offset)
        {
            return NullableHandle(offset);
        }

        //! \brief Convert to pointer. NullableHandle::Null() becomes nullptr.
        [[nodiscard]] inline void* ToPtr() const
        {
            return IsNull() ? nullptr : reinterpret_cast<void*>(m_Handle);
        }

        //! \brief Get underlying integral value.
        [[nodiscard]] inline USize ToOffset() const
        {
            return m_Handle;
        }

        //! \brief Check if the handle is NULL.
        [[nodiscard]] inline bool IsNull() const
        {
            return m_Handle == static_cast<USize>(-1);
        }

        //! \brief Check if the handle is not NULL.
        [[nodiscard]] inline bool IsValid() const
        {
            return m_Handle != static_cast<USize>(-1);
        }

        [[nodiscard]] inline explicit operator bool() const // NOLINT(google-explicit-constructor)
        {
            return IsValid();
        }

        //! \brief Reset the value to NULL.
        inline void Reset()
        {
            m_Handle = static_cast<USize>(-1);
        }

        friend bool operator==(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_Handle == rhs.m_Handle;
        }

        friend bool operator!=(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_Handle != rhs.m_Handle;
        }

        friend bool operator<(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_Handle < rhs.m_Handle;
        }

        friend bool operator>(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_Handle > rhs.m_Handle;
        }

        friend bool operator<=(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_Handle <= rhs.m_Handle;
        }

        friend bool operator>=(const NullableHandle& lhs, const NullableHandle& rhs)
        {
            return lhs.m_Handle >= rhs.m_Handle;
        }

        inline friend NullableHandle operator+(const NullableHandle& handle, USize offset)
        {
            return NullableHandle(handle.m_Handle + offset);
        }

        inline friend NullableHandle operator-(const NullableHandle& handle, USize offset)
        {
            return NullableHandle(handle.m_Handle - offset);
        }

        inline NullableHandle& operator+=(USize offset)
        {
            m_Handle += offset;
            return *this;
        }

        inline NullableHandle& operator-=(USize offset)
        {
            m_Handle -= offset;
            return *this;
        }
    };

    template<>
    inline NullableHandle AlignUp<NullableHandle, USize>(NullableHandle x, USize align)
    {
        return NullableHandle::FromOffset((x.ToOffset() + (align - 1u)) & ~(align - 1u));
    }
} // namespace FE

template<>
struct eastl::hash<FE::NullableHandle>
{
    inline size_t operator()(const FE::NullableHandle& handle) const noexcept
    {
        return eastl::hash<size_t>{}(handle.ToOffset());
    }
};
