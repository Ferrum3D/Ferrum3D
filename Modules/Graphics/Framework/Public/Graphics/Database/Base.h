#pragma once
#include <Shaders/Database/Base.h>

namespace FE::Graphics::DB
{
    struct Database;
    struct StoragePage;


    template<class T, uint32_t TOffset>
    struct ElementHandle final
    {
        const T* m_ptr = nullptr;

        void Setup(const std::byte* page, const uint32_t localRowIndex)
        {
            m_ptr = reinterpret_cast<const T*>(page + TOffset + localRowIndex * sizeof(T));
        }

        [[nodiscard]] const T& Get() const
        {
            return *m_ptr;
        }
    };


    template<class T, uint32_t TOffset>
    struct RWElementHandle final
    {
        T* m_ptr = nullptr;

        FE_FORCE_INLINE void Setup(std::byte* page, const uint32_t localRowIndex)
        {
            m_ptr = reinterpret_cast<T*>(page + TOffset + localRowIndex * sizeof(T));
        }

        template<class... TArgs>
        FE_FORCE_INLINE void Construct(TArgs&&... args) const
        {
            new (m_ptr) T(std::forward<TArgs>(args)...);
        }

        [[nodiscard]] FE_FORCE_INLINE T& Get() const
        {
            return *m_ptr;
        }
    };
} // namespace FE::Graphics::DB
