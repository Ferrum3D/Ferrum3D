#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/AllocatorBase.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Parallel/Interlocked.h>
#include <FeCore/Parallel/Mutex.h>

namespace FE
{
    namespace Internal
    {
        struct MonotonicBlockHeader
        {
            MonotonicBlockHeader* Next = nullptr;
            USize Size                 = 0;

            inline MonotonicBlockHeader(MonotonicBlockHeader* next, USize size)
                : Next(next)
                , Size(size)
            {
            }
        };
    } // namespace Internal

    struct MonotonicAllocatorDesc
    {
        IAllocator* ParentAllocator = nullptr; //!< Allocator used to allocate internal blocks. Defaults to HeapAllocator.
        UInt32 MaxBlockCount        = 64;      //!< Maximum number of blocks to allocate. Each new block will be twice as big
                                               //!< as previous block. Defaults to ParentAllocator.
        USize InitialBlockSize = 1024;         //!< Initial block size, will be two times larger for second block.
        USize MaxGrowCount     = 8;            //!< Block size will grow MaximumGrowCount at max.
    };

    template<class TMutex>
    class MonotonicAllocator : public AllocatorBase
    {
        using Header = Internal::MonotonicBlockHeader;

        MonotonicAllocatorDesc m_Desc;
        void* m_CurrentPointer;
        AtomicInt64 m_TotalUsage;
        Header* m_CurrentBlock;
        TMutex m_Mutex;

        inline Header* AllocateBlock();

    public:
        inline MonotonicAllocator();
        inline void Init(const MonotonicAllocatorDesc& desc);
        inline ~MonotonicAllocator() override;

        //! \brief Deallocate all previously allocated memory.
        //!
        //! If there were multiple blocks in the allocator, the function will keep only the first and delete
        //! the rest.
        inline void ResetAll();

        inline void* Allocate(USize size, USize alignment, const SourcePosition& position) override;
        inline void Deallocate(void* pointer, const SourcePosition& position, USize size) override;
        inline void* Reallocate(
            void* pointer, const SourcePosition& position, USize newSize, USize newAlignment, USize oldSize) override;
        inline USize SizeOfBlock(void* pointer) override;
        [[nodiscard]] USize TotalAllocated() const override;
    };

    template<class TMutex>
    MonotonicAllocator<TMutex>::MonotonicAllocator()
        : AllocatorBase("Monotonic allocator", "Local allocator that allocates monotonically from a large block")
        , m_CurrentPointer(nullptr)
        , m_TotalUsage(0)
        , m_CurrentBlock(nullptr)
    {
    }

    template<class TMutex>
    void MonotonicAllocator<TMutex>::Init(const MonotonicAllocatorDesc& desc)
    {
        m_Desc = desc;
        if (m_Desc.ParentAllocator == nullptr)
        {
            m_Desc.ParentAllocator = &GlobalAllocator<HeapAllocator>::Get();
        }

        m_CurrentBlock   = AllocateBlock();
        m_CurrentPointer = m_CurrentBlock + 1;
    }

    template<class TMutex>
    USize MonotonicAllocator<TMutex>::SizeOfBlock(void*)
    {
        return 0;
    }

    template<class TMutex>
    USize MonotonicAllocator<TMutex>::TotalAllocated() const
    {
        return static_cast<USize>(Interlocked::Load(m_TotalUsage));
    }

    template<class TMutex>
    void* MonotonicAllocator<TMutex>::Allocate(USize size, USize alignment, [[maybe_unused]] const SourcePosition& position)
    {
        Locker lk(m_Mutex);
        auto getMemoryLeft = [this](Header* block) {
            auto* blockEnd = reinterpret_cast<Int8*>(block) + block->Size + sizeof(Header);
            return blockEnd - reinterpret_cast<Int8*>(m_CurrentPointer);
        };

        auto* block = m_CurrentBlock;
        if (block && getMemoryLeft(block) < static_cast<SSize>(size))
        {
            block            = AllocateBlock();
            m_CurrentPointer = block + 1;
        }
        if (!block)
        {
            FE_CORE_ASSERT(false, "MonotonicAllocator is out of memory");
            return nullptr;
        }

        FE_PROFILE_ALLOC(size, alignment, position);
        void* result     = AlignUp(m_CurrentPointer, alignment);
        m_CurrentPointer = reinterpret_cast<Int8*>(result) + size;
        return result;
    }

    template<class TMutex>
    void MonotonicAllocator<TMutex>::Deallocate(void*, const SourcePosition&, USize)
    {
        // Do nothing here: the only way to free memory with this allocator is to ResetAll()
    }

    template<class TMutex>
    void* MonotonicAllocator<TMutex>::Reallocate(
        [[maybe_unused]] void* pointer, const SourcePosition& position, USize newSize, USize newAlignment,
        [[maybe_unused]] USize oldSize)
    {
        FE_PROFILE_REALLOC(pointer, position, newSize, newSlignment, oldSize);
        return Allocate(newSize, newAlignment, position);
    }

    template<class TMutex>
    Internal::MonotonicBlockHeader* MonotonicAllocator<TMutex>::AllocateBlock()
    {
        if (m_Desc.MaxBlockCount == 0)
        {
            return nullptr;
        }
        --m_Desc.MaxBlockCount;

        USize size = m_Desc.InitialBlockSize + sizeof(Header);
        if (m_Desc.MaxGrowCount > 0)
        {
            m_Desc.InitialBlockSize *= 2;
            --m_Desc.MaxGrowCount;
        }
        auto* result = new (m_Desc.ParentAllocator->Allocate(size, 16, FE_SRCPOS())) Header(m_CurrentBlock, size);
        return result;
    }

    template<class TMutex>
    void MonotonicAllocator<TMutex>::ResetAll()
    {
        auto* block      = m_CurrentBlock;
        auto* savedBlock = block;
        if (block)
        {
            block = block->Next;
        }
        while (block)
        {
            auto* current = block;
            block         = block->Next;
            auto size     = current->Size + sizeof(Header);
            FE_STATIC_SRCPOS(position);
            m_Desc.ParentAllocator->Deallocate(current, position, size);
        }

        m_CurrentBlock   = savedBlock;
        m_CurrentPointer = savedBlock + 1;
    }

    template<class TMutex>
    MonotonicAllocator<TMutex>::~MonotonicAllocator()
    {
        ResetAll();
        if (m_CurrentBlock)
        {
            auto size = m_CurrentBlock->Size + sizeof(Header);
            FE_STATIC_SRCPOS(position);
            m_Desc.ParentAllocator->Deallocate(m_CurrentBlock, position, size);
        }
    }

    //! \brief Synchronous monotonic allocator, doesn't use mutexes.
    using MonotonicAllocatorSync = MonotonicAllocator<NullMutex>;

    //! \brief Thread-safe monotonic allocator, uses a lock.
    using MonotonicAllocatorAsync = MonotonicAllocator<Mutex>;
} // namespace FE
