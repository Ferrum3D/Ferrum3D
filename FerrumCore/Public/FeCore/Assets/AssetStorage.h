﻿#pragma once
#include <FeCore/Assets/AssetCommon.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Parallel/SpinLock.h>

namespace FE::Assets
{
    class IAssetLoader;

    //! @brief Base class for all asset data types.
    //!
    //! Asset manager makes no assumptions on how the data of asset is stored. IAssetLoader must take
    //! a data stream and create an instance of this class. The users of asset will Get the data of assets
    //! they need in implementation-defined way through classes derived from AssetStorage.
    //! The AssetStorage class is responsible for reference counting and deleting the data after its reference count reaches 0.
    class AssetStorage
    {
        std::pmr::memory_resource* m_pAllocator = nullptr;
        uint32_t m_StrongRefCount = 0;
        uint32_t m_WeakRefCount = 0;

    protected:
        TracyLockable(SpinLock, m_Mutex);
        IAssetLoader* m_Loader = nullptr;

        //! @brief Delete the asset data, but keep this instance of AssetStorage.
        //!
        //! This function is called by AssetStorage when its reference count reaches 0.
        virtual void Delete() = 0;

    public:
        FE_RTTI_Class(AssetStorage, "46CA2164-383A-472F-995E-6FDBC9A1C550");

        //! @brief Construct an instance of AssetStorage with a loader.
        //!
        //! @param loader - Loader that will be used to load the asset data.
        inline explicit AssetStorage(IAssetLoader* loader, std::pmr::memory_resource* pAllocator = nullptr) noexcept
            : m_pAllocator(pAllocator)
            , m_Loader(loader)
        {
            if (m_pAllocator == nullptr)
                m_pAllocator = std::pmr::get_default_resource();
        }

        virtual ~AssetStorage() = default;

        //! @brief Check if the asset is still valid.
        [[nodiscard]] inline bool IsAlive() const
        {
            return m_StrongRefCount > 0;
        }

        //! @brief Returns true if the asset can be cached and shared and won't be loaded on each request.
        [[nodiscard]] inline virtual bool IsShareable()
        {
            return true;
        }

        //! @brief Returns true if at least some of the LODs or mip levels are loaded.
        [[nodiscard]] virtual bool IsAnythingLoaded() const = 0;

        //! @brief Returns true if the entire asset is loaded with all the sub-assets.
        [[nodiscard]] virtual bool IsCompletelyLoaded() const = 0;

        //! @brief Add a strong reference to the asset.
        inline void AddStrongRef()
        {
            std::unique_lock lk(m_Mutex);
            ++m_StrongRefCount;
            ++m_WeakRefCount;
        }

        //! @brief Remove a strong reference from the asset.
        inline void ReleaseStrongRef()
        {
            std::unique_lock lk(m_Mutex);

            if (--m_StrongRefCount == 0)
            {
                Delete();
            }
            if (--m_WeakRefCount == 0)
            {
                lk.unlock();
                Memory::Delete(m_pAllocator, this);
            }
        }

        //! @brief Add a weak reference to the asset.
        inline void AddWeakRef()
        {
            std::unique_lock lk(m_Mutex);
            ++m_WeakRefCount;
        }

        //! @brief Remove a weak reference from the asset.
        inline void ReleaseWeakRef()
        {
            std::unique_lock lk(m_Mutex);
            // Every strong reference holds a weak reference too, so if the weak counter reaches zero we can safely delete storage
            if (--m_WeakRefCount == 0)
            {
                lk.unlock();
                Memory::Delete(m_pAllocator, this);
            }
        }
    };
} // namespace FE::Assets
