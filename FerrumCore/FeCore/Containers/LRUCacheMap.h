#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    //! \brief Least Recently Used cache implementation.
    template<class TKey, class TValue>
    class LRUCacheMap final
    {
        UnorderedMap<TKey, typename LinkedList<std::pair<TKey, TValue>>::iterator> m_Map;
        LinkedList<std::pair<TKey, TValue>> m_Queue;
        USize m_Capacity = 0;

    public:
        FE_STRUCT_RTTI(LRUCacheMap, "6AB7529D-583B-4BE3-99E9-A2A3D2E73D2D");

        inline LRUCacheMap() = default;

        //! \brief Create the cache with initial capacity.
        //!
        //! \param [in] capacity - Initial capacity.
        inline explicit LRUCacheMap(USize capacity)
            : m_Capacity(capacity)
        {
        }

        //! \brief Get value by a key.
        //!
        //! \param [in] key - The key to find.
        //!
        //! \return Result value that is Result::Ok() if the value was in the cache.
        inline Result<TValue, bool> operator[](const TKey& key)
        {
            TValue value;
            if (TryGetValue(key, value))
            {
                return value;
            }

            // TODO: maybe use optional here
            return Err(false);
        }

        //!\ brief Change capacity of the cache, will remove least recently used when shrinking.
        //!
        //! \param [in] capacity - The new capacity.
        inline void SetCapacity(USize capacity)
        {
            m_Capacity = capacity;

            while (m_Map.size() > m_Capacity)
            {
                auto last = m_Queue.end();
                last--;
                m_Map.erase(last->first);
                m_Queue.pop_back();
            }
        }

        //! \brief Get cache capacity.
        [[nodiscard]] inline USize Capacity() const
        {
            return m_Capacity;
        }

        //! \brief Emplace a key-value pair to the cache.
        //!
        //! \param [in] key  - The key to associate the value with.
        //! \param [in] args - Arguments for value constructor.
        template<class... Args>
        inline void Emplace(const TKey& key, Args&&... args)
        {
            FE_CORE_ASSERT(m_Capacity > 0, "Cache capacity was zero");

            auto it = m_Map.find(key);
            m_Queue.emplace_front(std::piecewise_construct,
                                  std::forward_as_tuple(key),
                                  std::forward_as_tuple<Args>(std::forward<Args>(args)...)...);
            if (it != m_Map.end())
            {
                m_Queue.erase(it->second);
                m_Map.erase(it);
            }

            m_Map.emplace(key, m_Queue.begin());
            SetCapacity(m_Capacity);
        }

        //! \brief Check if a key is present, but do not promote to most recently used.
        //!
        //! \param [in] key - The key to check for.
        //!
        //! \return True if the key is in the cache.
        inline bool HasKey(const TKey& key)
        {
            auto it = m_Map.find(key);
            return it != m_Map.end();
        }

        //! \brief Try get a value from the cache.
        //!
        //! \param [in]  key   - The key to find.
        //! \param [out] value - The value that will be set on success.
        //!
        //! \return True if the key is in the cache.
        inline bool TryGetValue(const TKey& key, TValue& value)
        {
            auto it = m_Map.find(key);
            if (it != m_Map.end())
            {
                m_Queue.splice(m_Queue.begin(), m_Queue, it->second);
                value = m_Queue.begin()->second;
                return true;
            }

            return false;
        }
    };
} // namespace FE
