#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Memory/NullableHandle.h>

namespace FE
{
    //! \brief An entry in the \ref SparseSet.
    template<class TKey, class TValue>
    class SparseSetEntry
    {
        TKey m_Key;
        TValue m_Value;

    public:
        template<std::size_t I>
        std::tuple_element_t<I, SparseSetEntry>& get()
        {
            if constexpr (I == 0)
            {
                return m_Key;
            }

            if constexpr (I == 1)
            {
                return m_Value;
            }
        }

        inline SparseSetEntry() = default;

        inline SparseSetEntry(const TKey& key, const TValue& value)
            : m_Key(key)
            , m_Value(value)
        {
        }

        template<class... Args>
        inline explicit SparseSetEntry(const TKey& key, Args&&... args)
            : m_Key(key)
            , m_Value(std::forward<Args>(args)...)
        {
        }

        [[nodiscard]] inline const TKey& Key() const
        {
            return m_Key;
        }

        [[nodiscard]] inline TValue& Value()
        {
            return m_Value;
        }

        [[nodiscard]] inline const TValue& Value() const
        {
            return m_Value;
        }
    };

    template<class TKey, class TValue>
    class SparseSet final
    {
        List<USize> m_Sparse;
        List<SparseSetEntry<TKey, TValue>> m_Dense;

        inline NullableHandle DenseIndex(const TKey& key) const
        {
            auto denseIndex = m_Sparse[key];
            if (denseIndex < m_Dense.Size())
            {
                auto& entry = m_Dense[denseIndex];
                if (entry.Key() == key)
                {
                    return NullableHandle::FromOffset(denseIndex);
                }
            }

            return NullableHandle::Null();
        }

    public:
        FE_STRUCT_RTTI(SparseSet, "D2B6AC5C-E880-4307-8640-A6CAF80BE594");

        inline SparseSet() = default;

        //! \brief Create a SparseSet with initial capacity.
        //!
        //! \param [in] capacity - The capacity to initialize the SparseSet with.
        inline explicit SparseSet(USize capacity)
        {
            m_Dense.Reserve(capacity);
            m_Sparse.Resize(capacity, static_cast<USize>(-1));
        }

        //! \brief Get value by key.
        [[nodiscard]] inline TValue& operator[](const TKey& key)
        {
            auto index = DenseIndex(key);
            FE_CORE_ASSERT(index.IsValid(), "Key not found in the SparseSet");
            return m_Dense[index.ToOffset()].Value();
        }

        //! \brief Get value by key.
        [[nodiscard]] inline const TValue& operator[](const TKey& key) const
        {
            auto index = DenseIndex(key);
            FE_CORE_ASSERT(index.IsValid(), "Key not found in the SparseSet");
            return m_Dense[index.ToOffset()].Value();
        }

        //! \brief Add a value into the SparseSet.
        //!
        //! \param [in] key   - The key to associate the value with.
        //! \param [in] value - The value to add.
        //!
        //! \return True if the value was new, False if it was updated.
        inline bool Insert(const TKey& key, const TValue& value)
        {
            return Emplace(key, value);
        }

        //! \brief Add a value into the SparseSet.
        //!
        //! \param [in] key - The key to associate the value with.
        //!
        //! \return True if the value was new, False if it was updated.
        template<class... Args>
        inline bool Emplace(const TKey& key, Args&&... args)
        {
            FE_CORE_ASSERT(key < Capacity(), "SparseSet overflow");

            if (auto denseIndex = DenseIndex(key))
            {
                m_Dense[denseIndex.ToOffset()].Value() = TValue(std::forward<Args>(args)...);
                return false;
            }
            else
            {
                auto n = m_Dense.Size();
                m_Dense.Emplace(key, std::forward<Args>(args)...);
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
        //! \param [in] key    - The key to get.
        //! \param [out] value - The value that was found.
        //!
        //! \return True if the key was found.
        [[nodiscard]] inline bool TryGetAt(const TKey& key, TValue& value)
        {
            auto index = DenseIndex(key);
            if (index.IsNull())
            {
                return false;
            }

            value = m_Dense[index.ToOffset()].Value();
            return true;
        }

        //! \brief Remove a value from the SparseSet by key.
        //!
        //! \param [in] key - The key to remove.
        //!
        //! \return True if the key was found and removed.
        inline bool Remove(const TKey& key)
        {
            if (auto denseIndex = DenseIndex(key))
            {
                auto idx = denseIndex.ToOffset();
                m_Dense.SwapRemoveAt(idx);
                if (m_Dense.Any())
                {
                    m_Sparse[m_Dense[idx].Key()] = idx;
                }

                m_Sparse[key] = static_cast<USize>(-1);
                return true;
            }

            return false;
        }

        //! \brief Reset the SparseSet.
        //!
        //! Remove all elements and set a new capacity.
        //!
        //! \param [in] capacity - The new capacity.
        inline void Reset(USize capacity)
        {
            m_Dense.Clear();
            m_Dense.Shrink();
            m_Dense.Reserve(capacity);
            m_Sparse.Clear();
            m_Sparse.Shrink();
            m_Sparse.Resize(capacity, static_cast<USize>(-1));
        }

        [[nodiscard]] inline USize Size() const
        {
            return m_Dense.Size();
        }

        [[nodiscard]] inline USize Capacity() const
        {
            return m_Sparse.Size();
        }

        [[nodiscard]] inline auto begin()
        {
            return m_Dense.begin();
        }

        [[nodiscard]] inline auto end()
        {
            return m_Dense.end();
        }
    };
} // namespace FE

namespace std
{
    template<class TKey, class TValue>
    struct tuple_size<::FE::SparseSetEntry<TKey, TValue>> : integral_constant<size_t, 2>
    {
    };

    template<size_t I, class TKey, class TValue>
    struct tuple_element<I, ::FE::SparseSetEntry<TKey, TValue>> : tuple_element<I, tuple<const TKey, TValue>>
    {
    };
} // namespace std
