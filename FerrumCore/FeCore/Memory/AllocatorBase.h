#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/IAllocator.h>

namespace FE
{
    //! \brief Base class for all allocators. Helps implementing \ref IAllocator and \ref IAllocatorInfo.
    class AllocatorBase
        : public IAllocatorInfo
        , public IAllocator
    {
    protected:
        const char* m_Name;
        const char* m_Description;
        bool m_Initialized;

    public:
        FE_CLASS_RTTI(AllocatorBase, "C5139EB6-81F7-4F1A-B7E7-368DA47DFE32");

        //! \brief Sets name and description.
        //!
        //! \param [in] name        - Name of allocator.
        //! \param [in] description - Description of allocator.
        AllocatorBase(const char* name, const char* description) noexcept;

        inline virtual ~AllocatorBase() = default;

        //=========================================================================================
        // IAllocatorInfo

        [[nodiscard]] const char* GetName() const noexcept override;
        [[nodiscard]] const char* GetDescription() const noexcept override;
        [[nodiscard]] bool Initialized() const noexcept override;
        [[nodiscard]] IAllocator* Get() noexcept override;
        //=========================================================================================

        //=========================================================================================
        // IAllocator

        void CollectGarbage() override;
        [[nodiscard]] size_t MaxBlockSize() const override;
        //=========================================================================================

    protected:
        //! \brief Copy memory from source to destination.
        //!
        //! \param [out] destination - Where to write memory.
        //! \param [in]  source      - Where to read memory.
        //! \param [in]  byteSize    - Size of memory to copy in bytes.
        void CopyMemory(void* destination, void* source, size_t byteSize) const;

        //! \brief Profile memory allocation. Only useful if memory profiling if enabled.
        void ProfileAllocate(size_t size, size_t alignment, const SourcePosition& position);

        //! \brief Profile memory deallocation. Only useful if memory profiling if enabled.
        void ProfileDeallocate(void* pointer, const SourcePosition& position, size_t size);

        //! \brief Profile memory reallocation. Only useful if memory profiling if enabled.
        void ProfileReallocate(
            void* pointer, const SourcePosition& position, size_t newSize, size_t newAlignment, size_t oldSize);
    };

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
