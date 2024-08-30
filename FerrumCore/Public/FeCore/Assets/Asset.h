#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Assets/WeakAsset.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE
{
    //! @brief Asset holder.
    //!
    //! @tparam T - Asset storage type.
    //!
    //! This class is used to hold asset storage. It implements reference counting and
    //! provides interface for asset loading.
    template<class T>
    class Asset final
    {
        T* m_Storage = nullptr;

        explicit Asset(T* storage)
            : m_Storage(storage)
        {
            if (m_Storage)
            {
                m_Storage->AddStrongRef();
            }
        }

    public:
        Asset() = default;

        Asset(const Asset& other)
            : m_Storage(other.m_Storage)
        {
            if (m_Storage)
            {
                m_Storage->AddStrongRef();
            }
        }

        Asset(Asset&& other) noexcept
            : m_Storage(other.m_Storage)
        {
            other.m_Storage = nullptr;
        }

        Asset& operator=(const Asset& other)
        {
            Asset(other).Swap(*this);
            return *this;
        }

        Asset& operator=(Asset&& other) noexcept
        {
            Asset(std::move(other)).Swap(*this);
            return *this;
        }

        //! @brief Reset the asset holder and remove a reference from the asset.
        void Reset()
        {
            Asset{}.Swap(*this);
        }

        //! @brief Get underlying asset storage.
        T* Get() const
        {
            return m_Storage;
        }

        ~Asset()
        {
            if (m_Storage)
            {
                m_Storage->ReleaseStrongRef();
            }
        }

        void Swap(Asset& other)
        {
            T* t = other.m_Storage;
            other.m_Storage = m_Storage;
            m_Storage = t;
        }

        //! @brief Create a weak asset holder from this.
        WeakAsset<T> CreateWeak()
        {
            return WeakAsset<T>(m_Storage);
        }

        static Asset LoadSynchronously(Assets::IAssetManager* pAssetManager, Env::Name assetName)
        {
            const Env::Name assetType{ T::kAssetTypeName };
            const Assets::AssetLoadingFlags flags = Assets::AssetLoadingFlags::kSynchronous;
            T* storage = fe_assert_cast<T*>(pAssetManager->LoadAsset(assetName, assetType, flags));
            return Asset(storage);
        }

        T& operator*() const
        {
            FE_CORE_ASSERT(m_Storage, "Asset was empty");
            return *m_Storage;
        }

        T* operator->() const
        {
            return m_Storage;
        }

        //! @brief Check if the asset is valid.
        explicit operator bool() const
        {
            return m_Storage != nullptr;
        }
    };
} // namespace FE
