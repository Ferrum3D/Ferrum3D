#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE
{
    //! \brief An interface for main allocator functionality.
    //!
    //! This interface provides members like `Allocate()` and `Deallocate()`, but doesn't provide functions like `GetName()`.
    //! Allocator description, name and profiling are defined in \ref AllocatorBase class and \ref IAllocatorInfo.
    class IAllocator
    {
    public:
        FE_CLASS_RTTI(IAllocator, "F747D71B-3E32-43B0-84C7-DDA8377F5D8A");

        inline virtual ~IAllocator() = default;

        //! \brief Allocate a block of memory with specified size and alignment.
        //!
        //! \param [in] size      - Size of block to allocate.
        //! \param [in] alignment - Alignment of block to allocate.
        //! \param [in] position  - Position in source file where allocation was requested.
        //!
        //! \return The pointer to the allocated block.
        [[nodiscard]] virtual void* Allocate(USize size, USize alignment, const SourcePosition& position) = 0;

        //! \brief Deallocate memory at pointer.
        //!
        //! Size it optional here.
        //! Allocators must not rely on it, but can use this parameter only if it is non-zero. This can potentially
        //! speed up seeking for the block to free.\n
        //!
        //! Size parameter will also be used when memory profiling is enabled, so the size won't be requested with
        //! \ref IAllocator::SizeOfBlock().
        //!
        //! \param [in] pointer  - Pointer returned by IAllocator::Allocate function.
        //! \param [in] position - Position in source file where deallocation was requested.
        //! \param [in] size     - Size of allocated block (optional).
        virtual void Deallocate(void* pointer, const SourcePosition& position, USize size = 0) = 0;

        //! \brief Reallocate a block of memory.
        //!
        //! Use this to resize a block of memory and/or to change its alignment. Allocates memory block of at least new Size
        //! and copies old block contents to the new block. Can shrink or enlarge the block (depends on new and old size).
        //! Returns the same block without allocations if and only if the old and new sizes match.
        //!
        //! \param [in] pointer      - Pointer to the old block of memory.
        //! \param [in] position     - Position in source file where reallocation was requested.
        //! \param [in] newSize      - Size of block to allocate.
        //! \param [in] newAlignment - Alignment of block to allocate.
        //! \param [in] oldSize      - Size of already allocated block that will be deallocated.
        //!
        //! \return The pointer to the allocated block.
        [[nodiscard]] virtual void* Reallocate(
            void* pointer, const SourcePosition& position, USize newSize, USize newAlignment, USize oldSize = 0) = 0;

        //! \brief Query size of previously allocated block.
        //!
        //! This function's behavior is undefined if the pointer was allocated using a different allocator.
        //! It is allowed to return zero for some allocators, so don't rely on result if you don't know the
        //! exact allocator accessed through this interface. Pool and monotonic allocators usually work with
        //! very small allocations, so storing the size can make overhead very significant.
        //!
        //! \param [in] pointer - Pointer to a block of memory.
        //!
        //! \return Size of specified block.
        [[nodiscard]] virtual USize SizeOfBlock(void* pointer) = 0;

        //! \brief Try to free some unused memory from an allocator.
        //!
        //! Some memory can be too inefficient to free too often, but if the allocator fills a lot of memory with
        //! garbage it can be periodically collected. Note that this function doesn't always free actual memory since some
        //! allocators can't do that (for example a direct malloc-free wrapper).
        virtual void CollectGarbage() = 0;

        //! \brief Query total memory allocated through this allocator.
        //!
        //! \return Total allocated memory in bytes.
        [[nodiscard]] virtual USize TotalAllocated() const = 0;

        //! \brief Query maximum size of allocated block.
        //!
        //! Some allocators can have limits on allocation size. These allocators will return the value in bytes.
        //! For example, a pool allocator will return size of a single chunk.
        //! If an allocator doesn't have specific limits this function will return maximum value a USize can have.
        //!
        //! \return Maximum size of allocated block in bytes.
        [[nodiscard]] virtual USize MaxBlockSize() const = 0;
    };

    //! \brief Interface for allocator info.
    //!
    //! Provides information like name and description.
    class IAllocatorInfo
    {
    public:
        FE_CLASS_RTTI(IAllocatorInfo, "92D623F5-C9D4-41A3-9C99-101C81A93035");

        //! \brief Get name of allocator.
        [[nodiscard]] virtual const char* GetName() const noexcept = 0;

        //! \brief Get description of allocator.
        [[nodiscard]] virtual const char* GetDescription() const noexcept = 0;

        //! \brief Check if allocator is initialized.
        //!
        //! Allocators must be initialized before first use. Every allocator implementation provides
        //! a member function `Init(const Desc&)` and a type alias `using Desc = MyAllocatorDesc;`
        //!
        //! \return True if allocator was initialized and is ready to use.
        [[nodiscard]] virtual bool Initialized() const noexcept = 0;

        //! \brief Get allocator interface.
        //!
        //! \see IAllocator
        [[nodiscard]] virtual IAllocator* Get() noexcept = 0;
    };
} // namespace FE
