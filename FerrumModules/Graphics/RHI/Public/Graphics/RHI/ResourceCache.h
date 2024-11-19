#pragma once
#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/Memory.h>

namespace FE::Graphics::RHI
{
    struct ResourceCache final
    {
        ResourceCache() = default;

        void SetCapacity(size_t capacity)
        {
            m_cache.SetCapacity(capacity);
        }

        size_t Capacity() const
        {
            return m_cache.Capacity();
        }

        void AddObject(size_t descHash, Memory::RefCountedObjectBase* object)
        {
            m_cache.Emplace(descHash, object);
        }

        Memory::RefCountedObjectBase* FindObject(size_t descHash)
        {
            Rc<Memory::RefCountedObjectBase> result;
            if (m_cache.TryGetValue(descHash, result))
                return result.Get();

            return nullptr;
        }

    private:
        LRUCacheMap<size_t, Rc<Memory::RefCountedObjectBase>> m_cache;
    };
} // namespace FE::Graphics::RHI
