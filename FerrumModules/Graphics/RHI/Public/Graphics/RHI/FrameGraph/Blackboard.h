#pragma once
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/RHI/FrameGraph/Base.h>

namespace FE::Graphics::RHI
{
    struct FrameGraphBlackboard final
    {
        void Reset();

        template<class TPassData, class... TArgs>
        TPassData* Add(TArgs&&... args);

        template<class TPassData>
        TPassData* TryGet();

        template<class TPassData>
        TPassData* GetRequired();

    private:
        struct alignas(Memory::kDefaultAlignment) PassDataNode final
        {
            uint64_t m_typeHash;
            PassDataNode* m_next;
            void (*m_destructor)(void*);
        };

        Memory::LinearAllocator m_allocator;
        PassDataNode* m_head = nullptr;
    };


    inline void FrameGraphBlackboard::Reset()
    {
        PassDataNode* node = m_head;
        while (node != nullptr)
        {
            node->m_destructor(node + 1);
            node = node->m_next;
        }

        m_allocator.Clear();
        m_head = nullptr;
    }


    template<class TPassData, class... TArgs>
    TPassData* FrameGraphBlackboard::Add(TArgs&&... args)
    {
        void* memory = m_allocator.allocate(sizeof(TPassData) + sizeof(PassDataNode));

        PassDataNode* node = new (memory) PassDataNode();
        node->m_typeHash = TypeNameHash<TPassData>;
        node->m_next = m_head;
        node->m_destructor = [](void* ptr) {
            static_cast<TPassData*>(ptr)->~TPassData();
        };

        m_head = node;

        return new (node + 1) TPassData(std::forward<TArgs>(args)...);
    }


    template<class TPassData>
    TPassData* FrameGraphBlackboard::TryGet()
    {
        const uint64_t typeHash = TypeNameHash<TPassData>;

        PassDataNode* node = m_head;
        while (node != nullptr)
        {
            if (node->m_typeHash == typeHash)
                return reinterpret_cast<TPassData*>(node + 1);

            node = node->m_next;
        }

        return nullptr;
    }


    template<class TPassData>
    TPassData* FrameGraphBlackboard::GetRequired()
    {
        TPassData* data = TryGet<TPassData>();
        FE_Assert(data != nullptr);
        return data;
    }
} // namespace FE::Graphics::RHI
