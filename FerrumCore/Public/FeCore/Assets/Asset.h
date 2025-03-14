#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Assets/WeakAsset.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE
{
    //! @brief Asset holder.
    //!
    //! @tparam T  Asset storage type.
    //!
    //! This class is used to hold asset storage. It implements reference counting and
    //! provides interface for asset loading.
    template<class T>
    struct Asset final
    {
        Asset() = default;

        Asset(const Asset& other)
            : m_storage(other.m_storage)
        {
            if (m_storage)
                m_storage->AddStrongRef();
        }

        Asset(Asset&& other) noexcept
            : m_storage(other.m_storage)
        {
            other.m_storage = nullptr;
        }

        Asset& operator=(const Asset& other)
        {
            if (this != &other)
                Asset(other).Swap(*this);
            return *this;
        }

        Asset& operator=(Asset&& other) noexcept
        {
            Asset(festd::move(other)).Swap(*this);
            return *this;
        }

        //! @brief Reset the asset holder and remove a reference from the asset.
        void Reset()
        {
            Asset{}.Swap(*this);
        }

        //! @brief Get underlying asset storage.
        [[nodiscard]] T* Get() const
        {
            return m_storage;
        }

        ~Asset()
        {
            if (m_storage)
                m_storage->ReleaseStrongRef();
        }

        void Swap(Asset& other)
        {
            T* t = other.m_storage;
            other.m_storage = m_storage;
            m_storage = t;
        }

        //! @brief Create a weak asset holder from this.
        [[nodiscard]] WeakAsset<T> CreateWeak()
        {
            return WeakAsset<T>(m_storage);
        }

        [[nodiscard]] static Asset LoadSynchronously(Assets::IAssetManager* pAssetManager, Env::Name assetName)
        {
            const Env::Name assetType{ T::kAssetTypeName };
            const Assets::AssetLoadingFlags flags = Assets::AssetLoadingFlags::kSynchronous;
            T* storage = fe_assert_cast<T*>(pAssetManager->LoadAsset(assetName, assetType, flags));
            return Asset(storage);
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
            return m_storage != nullptr;
        }

    private:
        T* m_storage = nullptr;

        explicit Asset(T* storage)
            : m_storage(storage)
        {
            if (m_storage)
                m_storage->AddStrongRef();
        }
    };
} // namespace FE
