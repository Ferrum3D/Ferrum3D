#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Parallel/SpinLock.h>

namespace FE::Assets
{
    struct IAssetLoader;

    //! @brief Base class for all asset data types.
    //!
    //! Asset manager makes no assumptions on how the data of asset is stored. IAssetLoader must take
    //! a data stream and create an instance of this class. The users of asset will Get the data of assets
    //! they need in implementation-defined way through classes derived from AssetStorage.
    //! The AssetStorage class is responsible for reference counting and deleting the data after its reference count reaches 0.
    struct AssetStorage
    {
        FE_RTTI_Class(AssetStorage, "46CA2164-383A-472F-995E-6FDBC9A1C550");

        //! @brief Construct an instance of AssetStorage with a loader.
        //!
        //! @param loader - Loader that will be used to load the asset data.
        explicit AssetStorage(IAssetLoader* loader, std::pmr::memory_resource* allocator = nullptr) noexcept
            : m_allocator(allocator)
            , m_loader(loader)
        {
            if (m_allocator == nullptr)
                m_allocator = std::pmr::get_default_resource();
        }

        virtual ~AssetStorage() = default;

        //! @brief Check if the asset is still valid.
        [[nodiscard]] bool IsAlive() const
        {
            return m_strongRefCount > 0;
        }

        //! @brief Returns true if the asset can be cached and shared and won't be loaded on each request.
        [[nodiscard]] virtual bool IsShareable()
        {
            return true;
        }

        //! @brief Returns true if at least some of the LODs or mip levels are loaded.
        [[nodiscard]] virtual bool IsAnythingLoaded() const = 0;

        //! @brief Returns true if the entire asset is loaded with all the sub-assets.
        [[nodiscard]] virtual bool IsCompletelyLoaded() const = 0;

        //! @brief Add a strong reference to the asset.
        void AddStrongRef()
        {
            std::unique_lock lk(m_mutex);
            ++m_strongRefCount;
            ++m_weakRefCount;
        }

        //! @brief Remove a strong reference from the asset.
        void ReleaseStrongRef()
        {
            std::unique_lock lk(m_mutex);

            if (--m_strongRefCount == 0)
            {
                Delete();
            }
            if (--m_weakRefCount == 0)
            {
                lk.unlock();
                Memory::Delete(m_allocator, this);
            }
        }

        //! @brief Add a weak reference to the asset.
        void AddWeakRef()
        {
            std::unique_lock lk(m_mutex);
            ++m_weakRefCount;
        }

        //! @brief Remove a weak reference from the asset.
        void ReleaseWeakRef()
        {
            std::unique_lock lk(m_mutex);
            // Every strong reference holds a weak reference too, so if the weak counter reaches zero we can safely delete storage
            if (--m_weakRefCount == 0)
            {
                lk.unlock();
                Memory::Delete(m_allocator, this);
            }
        }

    private:
        std::pmr::memory_resource* m_allocator = nullptr;
        uint32_t m_strongRefCount = 0;
        uint32_t m_weakRefCount = 0;

    protected:
        mutable TracyLockable(Threading::SpinLock, m_mutex);
        IAssetLoader* m_loader = nullptr;

        //! @brief Delete the asset data, but keep this instance of AssetStorage.
        //!
        //! This function is called by AssetStorage when its reference count reaches 0.
        virtual void Delete() = 0;
    };
} // namespace FE::Assets
