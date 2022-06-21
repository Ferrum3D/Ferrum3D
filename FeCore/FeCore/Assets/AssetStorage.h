#pragma once
#include <FeCore/Assets/AssetCommon.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Parallel/SpinMutex.h>

namespace FE::Assets
{
    class IAssetLoader;

    //! \brief Base class for all asset data types.
    //!
    //! Asset manager makes no assumptions on how the data of asset is stored. IAssetLoader must take
    //! a data stream and create an instance of this class. The users of asset will Get the data of assets
    //! they need in implementation-defined way through classes derived from AssetStorage.
    class AssetStorage
    {
        SpinMutex m_Mutex;
        UInt32 m_StrongRefCount;
        UInt32 m_WeakRefCount;

    protected:
        IAssetLoader* m_Loader;

        //! \brief Delete the asset data, but keep this instance of AssetStorage.
        virtual void Delete() = 0;

    public:
        FE_CLASS_RTTI(AssetStorage, "46CA2164-383A-472F-995E-6FDBC9A1C550");

        inline explicit AssetStorage(IAssetLoader* loader) noexcept
            : m_Loader(loader)
            , m_WeakRefCount(0)
            , m_StrongRefCount(0)
        {
        }

        virtual ~AssetStorage() = default;

        [[nodiscard]] inline bool IsAlive() const
        {
            return m_StrongRefCount > 0;
        }

        //! \brief Returns true if the asset can be cached and shared and won't be loaded on each request.
        [[nodiscard]] inline virtual bool IsShareable()
        {
            return true;
        }

        [[nodiscard]] virtual AssetType GetAssetType() const = 0;

        inline void AddStrongRef()
        {
            UniqueLocker lk(m_Mutex);
            ++m_StrongRefCount;
            ++m_WeakRefCount;
        }

        inline void ReleaseStrongRef()
        {
            UniqueLocker lk(m_Mutex);

            if (--m_StrongRefCount == 0)
            {
                Delete();
            }
            if (--m_WeakRefCount == 0)
            {
                lk.Unlock();
                this->~AssetStorage();
                GlobalAllocator<HeapAllocator>::Get().Deallocate(this, FE_SRCPOS());
            }
        }

        inline void AddWeakRef()
        {
            UniqueLocker lk(m_Mutex);
            ++m_WeakRefCount;
        }

        inline void ReleaseWeakRef()
        {
            UniqueLocker lk(m_Mutex);
            // Every strong reference holds a weak reference too, so if the weak counter reaches zero we can safely delete storage
            if (--m_WeakRefCount == 0)
            {
                lk.Unlock();
                this->~AssetStorage();
                GlobalAllocator<HeapAllocator>::Get().Deallocate(this, FE_SRCPOS());
            }
        }
    };
} // namespace FE::Assets
