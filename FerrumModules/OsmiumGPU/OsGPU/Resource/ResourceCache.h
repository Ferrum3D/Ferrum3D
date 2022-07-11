#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>

namespace FE::Osmium
{
    class ResourceCache final
    {
        struct Node
        {
            IObject* Object;
            USize DescHash;

            inline Node(IObject* object, USize descHash)
                : Object(object)
                , DescHash(descHash)
            {
            }

            friend bool operator==(const Node& lhs, const Node& rhs)
            {
                return lhs.Object == rhs.Object && lhs.DescHash == rhs.DescHash;
            }
        };

        USize m_Capacity = 1024;
        Deque<Node> m_CacheQueue;
        UnorderedMap<USize, Node> m_CacheMap;

        inline void ReleaseObject()
        {
            Node* node = &m_CacheQueue.back();
            node->Object->ReleaseStrongRef();
            m_CacheMap.erase(node->DescHash);
            m_CacheQueue.pop_back();
        }

    public:
        inline ResourceCache() = default;

        inline ~ResourceCache()
        {
            Clear();
        }

        inline void SetCapacity(USize capacity)
        {
            while (m_CacheQueue.size() >= capacity)
            {
                ReleaseObject();
            }

            m_Capacity = capacity;
        }

        inline void Clear()
        {
            while (!m_CacheQueue.empty())
            {
                ReleaseObject();
            }
        }

        inline USize Capacity() const
        {
            return m_Capacity;
        }

        inline void AddObject(USize descHash, IObject* object)
        {
            object->AddStrongRef();
            if (m_CacheQueue.size() >= m_Capacity)
            {
                ReleaseObject();
            }

            FE_ASSERT_MSG(FindObject(descHash) == nullptr, "Object was already in the cache.");

            Node node(object, descHash);
            m_CacheQueue.push_front(node);
            m_CacheMap.emplace(descHash, node);
        }

        inline IObject* FindObject(USize descHash)
        {
            auto iter = m_CacheMap.find(descHash);
            if (iter == m_CacheMap.end())
            {
                return nullptr;
            }

            auto& node = iter->second;
            m_CacheQueue.erase(std::find(m_CacheQueue.begin(), m_CacheQueue.end(), node));
            m_CacheQueue.push_front(node);
            return node.Object;
        }
    };
} // namespace FE::Osmium
