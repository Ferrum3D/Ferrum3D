#pragma once
#include <FeCore/Assets/AssetStorage.h>

namespace FE::Assets
{
    template<class T>
    class WeakAsset final
    {
        T* m_Storage;

    public:
        FE_STRUCT_RTTI(WeakAsset, "1CCA3D88-4896-45E5-9A7F-12F338DA181A");

        inline WeakAsset() noexcept
            : m_Storage(nullptr)
        {
        }

        inline explicit WeakAsset(T* storage)
            : m_Storage(storage)
        {
            if (m_Storage)
            {
                m_Storage->AddWeakRef();
            }
        }

        inline WeakAsset(const WeakAsset& other)
            : m_Storage(other.m_Storage)
        {
            if (m_Storage)
            {
                m_Storage->AddWeakRef();
            }
        }

        inline WeakAsset(WeakAsset&& other) noexcept
            : m_Storage(other.m_Storage)
        {
            other.m_Storage = nullptr;
        }

        inline WeakAsset& operator=(const WeakAsset& other)
        {
            WeakAsset(other).Swap(*this);
            return *this;
        }

        inline WeakAsset& operator=(WeakAsset&& other) noexcept
        {
            WeakAsset(std::move(other)).Swap(*this);
            return *this;
        }

        inline void Reset()
        {
            WeakAsset{}.Swap(*this);
        }

        inline T* Get() noexcept
        {
            return m_Storage;
        }

        inline ~WeakAsset()
        {
            if (m_Storage)
            {
                m_Storage->ReleaseWeakRef();
            }
        }

        inline void Swap(WeakAsset& other) noexcept
        {
            auto* t         = other.m_Storage;
            other.m_Storage = m_Storage;
            m_Storage       = t;
        }

        inline T& operator*()
        {
            FE_CORE_ASSERT(m_Storage, "Asset was empty");
            return *m_Storage;
        }

        inline T* operator->()
        {
            return m_Storage;
        }

        inline const T& operator*() const
        {
            FE_CORE_ASSERT(m_Storage, "Asset was empty");
            return *m_Storage;
        }

        inline const T* operator->() const
        {
            return *m_Storage;
        }

        inline operator bool() const
        {
            return m_Storage && m_Storage->IsAlive();
        }
    };
} // namespace FE::Assets
