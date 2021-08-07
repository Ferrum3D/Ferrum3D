#pragma once
#include <FeCore/Memory/IAllocator.h>
#include <FeCore/Utils/CoreUtils.h>

namespace FE
{
    /**
     * @brief Base class for all allocators. Helps implementing IAllocator and IAllocatorInfo
    */
    class AllocatorBase
        : public IAllocatorInfo
        , public IAllocator
    {
    protected:
        const char* m_Name;
        const char* m_Description;
        bool m_Initialized;

    public:
        /**
         * @brief Sets name and description.
        */
        AllocatorBase(const char* name, const char* description) noexcept;

        inline virtual ~AllocatorBase() = default;

        //=========================================================================================
        // IAllocatorInfo

        const char* GetName() const noexcept override;
        const char* GetDescription() const noexcept override;
        bool Initialized() const noexcept override;
        virtual IAllocator* Get() noexcept override;
        //=========================================================================================

        //=========================================================================================
        // IAllocator

        virtual void CollectGarbage() override;
        virtual size_t MaxBlockSize() const override;
        //=========================================================================================

    protected:
        void CopyMemory(void* destination, void* source, size_t byteSize) const;

        void ProfileAllocate(size_t size, size_t alignment, const SourcePosition& position);
        void ProfileDeallocate(void* pointer, const SourcePosition& position, size_t size);

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
#     define FE_PROFILE_ALLOC(size, alignment, position) do {} while (0)
#     define FE_PROFILE_DEALLOC(pointer, position, size) do {} while (0)
#     define FE_PROFILE_REALLOC(pointer, position, newSize, newAlignment, oldSize) do {} while (0)
#endif
    // clang-format on
} // namespace FE
