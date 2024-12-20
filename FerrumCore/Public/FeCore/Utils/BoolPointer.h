﻿#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    //! @brief Stores a boolean-pointer pair in 8 bytes instead of 9.
    //!
    //! Every pointer allocated with rpmalloc is 16-bytes aligned, so the 4 least significant bits
    //! of the pointer will always be 0. The BoolPointer class uses the fact to store a boolean
    //! value in the least significant bit. We don't use other 3 bits here, because we want other
    //! types of alignment to work with this class too. Only the HeapAllocator can guarantee 16-byte
    //! alignment.
    //!
    //! @tparam T Type of pointer to store.
    template<class T>
    class BoolPointer
    {
        std::atomic<uint64_t> m_Data = 0;

        inline static constexpr uint64_t PointerMask = static_cast<uint64_t>(-1) ^ 1;
        inline static constexpr uint64_t BooleanMask = ~PointerMask;

    public:
        FE_FORCE_INLINE BoolPointer() = default;

        FE_FORCE_INLINE BoolPointer(T* pointer, bool boolean)
        {
            m_Data = reinterpret_cast<int64_t>(pointer) | (boolean ? 1 : 0);
        }

        FE_FORCE_INLINE void SetPointer(T* pointer)
        {
            const uint64_t value = m_Data & BooleanMask;
            m_Data = value | reinterpret_cast<int64_t>(pointer);
        }

        FE_FORCE_INLINE void SetBool(bool boolean)
        {
            const uint64_t value = m_Data & PointerMask;
            m_Data = value | (boolean ? 1 : 0);
        }

        [[nodiscard]] FE_FORCE_INLINE T* GetPointer() const
        {
            return reinterpret_cast<T*>(m_Data & PointerMask);
        }

        [[nodiscard]] FE_FORCE_INLINE bool GetBool() const
        {
            return m_Data & BooleanMask;
        }
    };
} // namespace FE
