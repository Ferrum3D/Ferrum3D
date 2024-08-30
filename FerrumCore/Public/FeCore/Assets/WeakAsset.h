#pragma once
#include <FeCore/Assets/AssetStorage.h>

namespace FE
{
    //! @brief A weak asset holder.
    //!
    //! @tparam T - Asset storage type.
    //!
    //! This class is used to hold a weak reference to asset storage. When the asset is no longer valid,
    //! the storage will be deleted, but all the weak references will remain.
    //! It can be used by IAssetManager for caching of assets.
    template<class T>
    class WeakAsset final
    {
        T* m_Storage;

    public:
        FE_RTTI_Base(WeakAsset, "1CCA3D88-4896-45E5-9A7F-12F338DA181A");

        inline WeakAsset() noexcept
            : m_Storage(nullptr)
        {
        }

        //! @brief Create from preloaded asset.
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

        //! @brief Get underlying asset storage.
        inline T* Get() const
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

        //! @brief Swap two asset holders.
        inline void Swap(WeakAsset& other) noexcept
        {
            auto* t = other.m_Storage;
            other.m_Storage = m_Storage;
            m_Storage = t;
        }

        inline T& operator*() const
        {
            FE_CORE_ASSERT(m_Storage, "Asset was empty");
            return *m_Storage;
        }

        inline T* operator->() const
        {
            return m_Storage;
        }

        //! @brief Check if the asset is valid.
        inline explicit operator bool() const
        {
            return m_Storage && m_Storage->IsAlive();
        }
    };
} // namespace FE
