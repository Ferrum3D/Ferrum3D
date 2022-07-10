#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/IAllocator.h>

namespace FE
{
    //! \brief Base class for all allocators. Helps implementing \ref IAllocator and \ref IAllocatorInfo.
    template<class THandle>
    class GenericAllocatorBase
        : public IAllocatorInfo
        , public IGenericAllocator<THandle>
    {
    protected:
        const char* m_Name;
        const char* m_Description;
        bool m_Initialized;

    public:
        FE_CLASS_RTTI(GenericAllocatorBase, "C5139EB6-81F7-4F1A-B7E7-368DA47DFE32");

        //! \brief Sets name and description.
        //!
        //! \param [in] name        - Name of allocator.
        //! \param [in] description - Description of allocator.
        GenericAllocatorBase(const char* name, const char* description) noexcept;

        inline ~GenericAllocatorBase() override = default;

        //=========================================================================================
        // IAllocatorInfo

        [[nodiscard]] const char* GetName() const noexcept override;
        [[nodiscard]] const char* GetDescription() const noexcept override;
        [[nodiscard]] bool Initialized() const noexcept override;
        //=========================================================================================

        //=========================================================================================
        // IAllocator

        void CollectGarbage() override;
        [[nodiscard]] USize MaxBlockSize() const override;
        //=========================================================================================

    protected:
        //! \brief Copy memory from source to destination.
        //!
        //! \param [out] destination - Where to write memory.
        //! \param [in]  source      - Where to read memory.
        //! \param [in]  byteSize    - Size of memory to copy in bytes.
        static void CopyMemory(THandle destination, THandle source, size_t byteSize);

        //! \brief Profile memory allocation. Only useful if memory profiling if enabled.
        void ProfileAllocate(size_t size, size_t alignment, const SourcePosition& position);

        //! \brief Profile memory deallocation. Only useful if memory profiling if enabled.
        void ProfileDeallocate(THandle pointer, const SourcePosition& position, size_t size);

        //! \brief Profile memory reallocation. Only useful if memory profiling if enabled.
        void ProfileReallocate(
            THandle pointer, const SourcePosition& position, size_t newSize, size_t newAlignment, size_t oldSize);
    };

    template<class THandle>
    inline GenericAllocatorBase<THandle>::GenericAllocatorBase(const char* name, const char* description) noexcept
        : m_Name(name)
        , m_Description(description)
        , m_Initialized(false)
    {
    }

    template<class THandle>
    inline const char* GenericAllocatorBase<THandle>::GetName() const noexcept
    {
        return m_Name;
    }

    template<class THandle>
    inline const char* GenericAllocatorBase<THandle>::GetDescription() const noexcept
    {
        return m_Description;
    }

    template<class THandle>
    inline bool GenericAllocatorBase<THandle>::Initialized() const noexcept
    {
        return m_Initialized;
    }

    template<class THandle>
    inline void GenericAllocatorBase<THandle>::CollectGarbage()
    {
    }

    template<class THandle>
    inline USize GenericAllocatorBase<THandle>::MaxBlockSize() const
    {
        return static_cast<size_t>(-1);
    }

    template<class THandle>
    inline void GenericAllocatorBase<THandle>::CopyMemory(THandle destination, THandle source, size_t byteSize)
    {
        memcpy(destination, source, byteSize);
    }

    template<class THandle>
    inline void GenericAllocatorBase<THandle>::ProfileAllocate(size_t size, size_t alignment, const SourcePosition& position)
    {
        (void)size;
        (void)alignment;
        (void)position;
        // TODO: in-engine memory profiler
    }

    template<class THandle>
    inline void GenericAllocatorBase<THandle>::ProfileDeallocate(THandle pointer, const SourcePosition& position, size_t size)
    {
        (void)position;
        if (size == 0)
            size = SizeOfBlock(pointer);
        // TODO: in-engine memory profiler
    }

    template<class THandle>
    inline void GenericAllocatorBase<THandle>::ProfileReallocate(
        THandle pointer, const SourcePosition& position, size_t newSize, size_t newAlignment, size_t oldSize)
    {
        (void)position;
        (void)newSize;
        (void)newAlignment;
        if (oldSize == 0)
            oldSize = SizeOfBlock(pointer);
        // TODO: in-engine memory profiler
    }

    using AllocatorBase = GenericAllocatorBase<void*>;

    // clang-format off
#ifdef FE_MEMORY_PROFILE
#     define FE_PROFILE_ALLOC(size, alignment, position)                                                                         \
    do { ProfileAllocate(size, alignment, position); } while(0)
#     define FE_PROFILE_DEALLOC(pointer, position, size)                                                                         \
    do { ProfileDeallocate(pointer, position, size); } while(0)
#     define FE_PROFILE_REALLOC(pointer, position, newSize, newAlignment, oldSize)                                               \
    do { ProfileReallocate(pointer, position, newSize, newAlignment, oldSize); } while (0)
#else
    //! \brief Profile memory allocation. Only useful if memory profiling if enabled.
    //! 
    //! \note This can only be called from an allocator that inherits \ref AllocatorBase.
#     define FE_PROFILE_ALLOC(size, alignment, position) do {} while (0)
    //! \brief Profile memory deallocation. Only useful if memory profiling if enabled.
    //! 
    //! \note This can only be called from an allocator that inherits \ref AllocatorBase.
#     define FE_PROFILE_DEALLOC(pointer, position, size) do {} while (0)
    //! \brief Profile memory reallocation. Only useful if memory profiling if enabled.
    //! 
    //! \note This can only be called from an allocator that inherits \ref AllocatorBase.
#     define FE_PROFILE_REALLOC(pointer, position, newSize, newAlignment, oldSize) do {} while (0)
#endif
    // clang-format on
} // namespace FE
