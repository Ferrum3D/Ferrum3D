#pragma once
#include <FeCore/Assets/AssetStorage.h>

namespace FE
{
    //! @brief A weak asset holder.
    //!
    //! @tparam T  Asset storage type.
    //!
    //! This class is used to hold a weak reference to asset storage. When the asset is no longer valid,
    //! the storage will be deleted, but all the weak references will remain.
    //! It can be used by IAssetManager for caching of assets.
    template<class T>
    struct WeakAsset final
    {
        FE_RTTI_Base(WeakAsset, "1CCA3D88-4896-45E5-9A7F-12F338DA181A");

        WeakAsset() noexcept
            : m_storage(nullptr)
        {
        }

        //! @brief Create from preloaded asset.
        explicit WeakAsset(T* storage)
            : m_storage(storage)
        {
            if (m_storage)
                m_storage->AddWeakRef();
        }

        WeakAsset(const WeakAsset& other)
            : m_storage(other.m_storage)
        {
            if (m_storage)
                m_storage->AddWeakRef();
        }

        WeakAsset(WeakAsset&& other) noexcept
            : m_storage(other.m_storage)
        {
            other.m_storage = nullptr;
        }

        WeakAsset& operator=(const WeakAsset& other)
        {
            if (this != &other)
                WeakAsset(other).Swap(*this);
            return *this;
        }

        WeakAsset& operator=(WeakAsset&& other) noexcept
        {
            WeakAsset(festd::move(other)).Swap(*this);
            return *this;
        }

        void Reset()
        {
            WeakAsset{}.Swap(*this);
        }

        //! @brief Get underlying asset storage.
        [[nodiscard]] T* Get() const
        {
            return m_storage;
        }

        ~WeakAsset()
        {
            if (m_storage)
                m_storage->ReleaseWeakRef();
        }

        //! @brief Swap two asset holders.
        void Swap(WeakAsset& other) noexcept
        {
            auto* t = other.m_storage;
            other.m_storage = m_storage;
            m_storage = t;
        }

        T& operator*() const
        {
            FE_Assert(m_storage, "Asset was empty");
            return *m_storage;
        }

        T* operator->() const
        {
            return m_storage;
        }

        //! @brief Check if the asset is valid.
        explicit operator bool() const
        {
            return m_storage && m_storage->IsAlive();
        }

    private:
        T* m_storage;
    };
} // namespace FE
