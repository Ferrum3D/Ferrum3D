#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Memory/NullableHandle.h>

namespace FE
{
    //! \brief Describes how SparseStorage will allocate its memory.
    enum class SparseStorageAllocationPolicy
    {
        None,             //!< Invalid value that indicates that the storage was not initialized.
        PreAllocate,      //!< Pre-allocate all the memory.
        FirstUseAllocate, //!< Lazily allocate all memory on first use.
        Dynamic           //!< Dynamically resize underlying List when out of memory.
    };

    //! \brief Sparse storage descriptor.
    struct SparseStorageDesc
    {
        USize ValueByteAlignment; //!< Alignment of value to store in bytes.
        USize ValueByteSize;      //!< Size of value to store in bytes.
        USize Capacity;           //!< Capacity of the storage as number of stored values.

        SparseStorageAllocationPolicy AllocationPolicy; //!< Storage allocation policy.

        inline SparseStorageDesc()
            : ValueByteAlignment(0)
            , ValueByteSize(0)
            , Capacity(0)
            , AllocationPolicy()
        {
        }

        inline SparseStorageDesc(USize alignment, USize valueSize,
                                 SparseStorageAllocationPolicy allocationPolicy = SparseStorageAllocationPolicy::PreAllocate,
                                 USize capacity                                 = 256)
        {
            ValueByteAlignment = alignment;
            ValueByteSize      = valueSize;
            Capacity           = capacity;
            AllocationPolicy   = allocationPolicy;
        }
    };

    //! \brief Same as SparseSet<T>, but for arbitrary value sizes.
    //!
    //! This class stores a specified number of chunks that have the following memory layout:
    //! ```
    //!              +---->  +-------+ <--- Chunk pointer, aligned to specified alignment
    //!              |       | Value |
    //!  m_ChunkSize |       |-------| <--- Chunk pointer + m_KeyOffset, aligned to alignof(TKey)
    //!              |       |  Key  |
    //!              +---->  +-------+ <--- End pointer, aligned to specified alignment
    //!                      | Value |
    //!                         ...
    //! ```
    //!
    //! Note that the destructors of stored object won't be called, so the stored values
    //! _must be trivially copyable and trivially destructible or must be destructed manually_.
    template<class TKey>
    class SparseStorage final
    {
        SparseStorageDesc m_Desc;

        SSize m_ChunkSize = -1;
        SSize m_KeyOffset = -1;

        List<USize> m_Sparse;
        List<Int8> m_Dense;

        [[nodiscard]] inline void* GetEntryValue(USize index)
        {
            return &m_Dense[index * m_ChunkSize];
        }

        [[nodiscard]] inline const TKey& GetEntryKey(USize index) const
        {
            const void* pointer = &m_Dense[index * m_ChunkSize + m_KeyOffset];
            return *static_cast<const TKey*>(pointer);
        }

        [[nodiscard]] inline TKey& GetEntryKey(USize index)
        {
            void* pointer = &m_Dense[index * m_ChunkSize + m_KeyOffset];
            return *static_cast<TKey*>(pointer);
        }

        inline NullableHandle DenseIndex(const TKey& key) const
        {
            auto denseIndex = m_Sparse[key];
            if (denseIndex < m_Dense.Size())
            {
                if (GetEntryKey(denseIndex) == key)
                {
                    return NullableHandle::FromOffset(denseIndex);
                }
            }

            return NullableHandle::Null();
        }

    public:
        FE_STRUCT_RTTI(SparseStorage, "41289318-36F5-41D6-BFED-73C7DC58C695");

        inline SparseStorage() = default;

        //! \brief Initialize the storage and allocate memory.
        //!
        //! \see SparseStorageDesc
        //!
        //! \param [in] alignment - The alignment required for the stored value.
        //! \param [in] valueSize - The size of the value to store.
        //! \param [in] capacity  - The number of chunks to allocate memory for.
        inline void Init(const SparseStorageDesc& desc)
        {
            FE_CORE_ASSERT(m_ChunkSize == -1, "Sparse storage can only be initialized once");
            m_Desc = desc;

            m_KeyOffset = AlignUp<alignof(TKey)>(desc.ValueByteSize);
            m_ChunkSize = AlignUp(m_KeyOffset + sizeof(TKey), desc.ValueByteAlignment);

            if (desc.AllocationPolicy == SparseStorageAllocationPolicy::PreAllocate)
            {
                m_Sparse.Resize(desc.Capacity, static_cast<USize>(-1));
                m_Dense.Reserve(desc.Capacity * m_ChunkSize);
            }
        }

        //! \brief Add a value into the SparseStorage.
        //!
        //! \param [in] key   - The key to associate the value with.
        //! \param [in] value - The pointer to the value.
        //!
        //! \return True if the value was new, False if it was updated.
        inline bool Insert(const TKey& key, void* value)
        {
            FE_CORE_ASSERT(key < m_Desc.Capacity, "SparseStorage overflow");

            if (m_Desc.AllocationPolicy == SparseStorageAllocationPolicy::FirstUseAllocate && Capacity() == 0)
            {
                m_Sparse.Resize(m_Desc.Capacity, static_cast<USize>(-1));
                m_Dense.Reserve(m_Desc.Capacity * m_ChunkSize);
            }

            if (auto denseIndex = DenseIndex(key))
            {
                memcpy(GetEntryValue(denseIndex.ToOffset()), value, m_Desc.ValueByteSize);
                return false;
            }
            else
            {
                if (m_Desc.AllocationPolicy == SparseStorageAllocationPolicy::Dynamic && m_Sparse.Size() <= key)
                {
                    m_Sparse.Append(key - m_Sparse.Size() + 1, static_cast<USize>(-1));
                }

                auto n = m_Dense.Size() / m_ChunkSize;
                m_Dense.Append(m_ChunkSize, -1);
                GetEntryKey(n) = key;
                memcpy(GetEntryValue(n), value, m_Desc.ValueByteSize);
                m_Sparse[key] = n;
                return true;
            }
        }

        //! \brief Check if the key is present.
        //!
        //! \param [in] key - The key to check for.
        //!
        //! \return True if the was found.
        [[nodiscard]] inline bool Contains(const TKey& key) const
        {
            return DenseIndex(key).IsValid();
        }

        //! \brief Try to get value by key.
        //!
        //! \param [in] key - The key to get.
        [[nodiscard]] inline void* TryGetAt(const TKey& key)
        {
            auto index = DenseIndex(key);
            return index.IsValid() ? GetEntryValue(index.ToOffset()) : nullptr;
        }

        //! \brief Remove a value from the SparseStorage by key.
        //!
        //! \param [in] key - The key to remove.
        //!
        //! \return True if the key was found and removed.
        inline bool Remove(const TKey& key)
        {
            if (auto denseIndex = DenseIndex(key))
            {
                auto idx = denseIndex.ToOffset();
                if (m_Dense.Size() > 1)
                {
                    memcpy(GetEntryValue(idx), GetEntryValue(m_Dense.Size() - m_ChunkSize), m_ChunkSize);
                    m_Dense.Resize(m_Dense.Size() - m_ChunkSize);
                    m_Sparse[GetEntryKey(idx)] = idx;
                }

                m_Sparse[key] = static_cast<USize>(-1);
                return true;
            }

            return false;
        }

        //! \brief Release unused memory.
        //!
        //! If the storage uses SparseStorageAllocationPolicy::Dynamic, this function will shrink
        //! internal data structures.<br>
        //! If it uses SparseStorageAllocationPolicy::FirstUseAllocate, it will clear the lists if Size() == 0.<br>
        //! If the policy is SparseStorageAllocationPolicy::PreAllocate, it will do nothing.
        inline void Shrink()
        {
            if (m_Desc.AllocationPolicy == SparseStorageAllocationPolicy::PreAllocate)
            {
                return;
            }

            if (m_Desc.AllocationPolicy == SparseStorageAllocationPolicy::FirstUseAllocate)
            {
                if (m_Dense.Size() == 0)
                {
                    m_Dense.Clear();
                    m_Dense.Shrink();

                    m_Sparse.Clear();
                    m_Sparse.Shrink();
                }

                return;
            }

            m_Dense.Shrink();

            USize lastIndex = 0;
            for (USize i = 0; i < m_Sparse.Size(); ++i)
            {
                if (Contains(static_cast<TKey>(i)))
                {
                    lastIndex = i;
                }
            }

            m_Sparse.Resize(lastIndex + 1);
        }

        [[nodiscard]] inline USize Size() const
        {
            return m_Dense.Size() / m_ChunkSize;
        }

        [[nodiscard]] inline USize Capacity() const
        {
            return m_Sparse.Size();
        }
    };
} // namespace FE
