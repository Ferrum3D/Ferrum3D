#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Assets/WeakAsset.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE::Assets
{
    //! \brief Asset holder.
    //!
    //! \tparam T - Asset storage type.
    //!
    //! This class is used to hold asset storage. It implements reference counting and
    //! provides interface for asset loading.
    template<class T>
    class Asset final
    {
        AssetID m_ID;
        T* m_Storage;

    public:
        FE_STRUCT_RTTI(Asset, "A0DD3482-B26A-498A-AB03-F40642D50D5F");

        //! \brief Create an empty asset from its ID.
        //!
        //! The asset will be empty until loading will be requested.
        //!
        //! \param [in] assetID - ID of asset to hold.
        inline explicit Asset(const AssetID& assetID)
            : m_Storage(nullptr)
            , m_ID(assetID)
        {
        }

        //! \brief Create from preloaded asset
        //!
        //! \param [in] assetID - ID of asset to hold.
        //! \param [in] storage - Preloaded asset storage.
        inline Asset(const AssetID& assetID, T* storage)
            : m_Storage(storage)
            , m_ID(assetID)
        {
            if (m_Storage)
            {
                m_Storage->AddStrongRef();
            }
        }

        //! \brief Copy constructor.
        inline Asset(const Asset& other)
            : m_Storage(other.m_Storage)
        {
            if (m_Storage)
            {
                m_Storage->AddStrongRef();
            }
        }

        //! \brief Move constructor.
        inline Asset(Asset&& other) noexcept
            : m_Storage(other.m_Storage)
        {
            other.m_Storage = nullptr;
        }

        //! \brief Copy assignment.
        inline Asset& operator=(const Asset& other)
        {
            Asset(other).Swap(*this);
            return *this;
        }

        //! \brief Move assignment.
        inline Asset& operator=(Asset&& other) noexcept
        {
            Asset(std::move(other)).Swap(*this);
            return *this;
        }

        //! \brief Reset the asset holder and remove a reference from the asset.
        inline void Reset()
        {
            Asset{}.Swap(*this);
        }

        //! \brief Get underlying asset storage.
        inline T* Get()
        {
            return m_Storage;
        }

        inline ~Asset()
        {
            if (m_Storage)
            {
                m_Storage->ReleaseStrongRef();
            }
        }

        //! \brief Swap two asset holders.
        inline void Swap(Asset& other)
        {
            auto* t         = other.m_Storage;
            other.m_Storage = m_Storage;
            m_Storage       = t;
        }

        //! \brief Create a weak asset holder from this.
        inline WeakAsset<T> CreateWeak()
        {
            return WeakAsset<T>(m_Storage);
        }

        //! \brief Load asset from the manager synchronously.
        //!
        //! The function will block until the asset is loaded. It will call the manager's LoadAsset function.
        //!
        //! \see IAssetManager::LoadAsset
        inline void LoadSync()
        {
            m_Storage = static_cast<T*>(ServiceLocator<IAssetManager>::Get()->LoadAsset(m_ID));
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

        //! \brief Check if the asset is valid.
        inline explicit operator bool() const
        {
            return m_Storage;
        }
    };
} // namespace FE::Assets
