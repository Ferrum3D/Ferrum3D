#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/Memory/Memory.h>

namespace FE::Osmium
{
    class ResourceCache final
    {
        LRUCacheMap<USize, Rc<IObject>> m_Cache;

    public:
        inline ResourceCache() = default;

        inline void SetCapacity(USize capacity)
        {
            m_Cache.SetCapacity(capacity);
        }

        inline USize Capacity() const
        {
            return m_Cache.Capacity();
        }

        inline void AddObject(USize descHash, IObject* object)
        {
            m_Cache.Emplace(descHash, object);
        }

        inline IObject* FindObject(USize descHash)
        {
            Rc<IObject> result;
            if (m_Cache.TryGetValue(descHash, result))
            {
                return result.Get();
            }

            return nullptr;
        }
    };
} // namespace FE::Osmium
