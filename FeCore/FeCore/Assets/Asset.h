#pragma once
#include <FeCore/Assets/WeakAsset.h>
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Modules/Singleton.h>

namespace FE::Assets
{
    template<class T>
    class Asset final
    {
        T* m_Storage;
        AssetID m_ID;

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

        inline Asset(const Asset& other)
            : m_Storage(other.m_Storage)
        {
            if (m_Storage)
            {
                m_Storage->AddStrongRef();
            }
        }

        inline Asset(Asset&& other) noexcept
            : m_Storage(other.m_Storage)
        {
            other.m_Storage = nullptr;
        }

        inline Asset& operator=(const Asset& other)
        {
            Asset(other).Swap(*this);
            return *this;
        }

        inline Asset& operator=(Asset&& other) noexcept
        {
            Asset(std::move(other)).Swap(*this);
            return *this;
        }

        inline void Reset()
        {
            Asset{}.Swap(*this);
        }

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

        inline void Swap(Asset& other)
        {
            auto* t         = other.m_Storage;
            other.m_Storage = m_Storage;
            m_Storage       = t;
        }

        inline WeakAsset<T> CreateWeak()
        {
            return WeakAsset<T>(m_Storage);
        }

        inline void LoadSync()
        {
            m_Storage = Singleton<IAssetManager>::Get()->LoadAsset(m_ID);
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
            return m_Storage;
        }
    };
} // namespace FE::Assets