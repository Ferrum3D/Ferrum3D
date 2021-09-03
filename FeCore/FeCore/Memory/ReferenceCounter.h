#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/IAllocator.h>
#include <FeCore/Parallel/Interlocked.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE
{
    class IObject;

    //! \brief The reference counter that holds the number of references to the object.
    //!
    //! This class holds number of references to the object and a pointer to the allocator that
    //! was used to allocate this object. It assumes the following memory layout:
    //!
    //!     +------------------+ <--- this pointer
    //!     | ReferenceCounter |
    //!     --------------------
    //!     |      Object      |
    //!     +------------------+
    //!
    //! It will delete `this` assumming that a single block was used to allocate the object and the counter.\n
    //!
    //! Example (pseudo-code):
    //! \code{.cpp}
    //!     class MyObject : Object<IObject> {};
    //!
    //!     ReferenceCounter* rc = malloc(sizeof(ReferenceCounter) + sizeof(MyObject));
    //!     MyObject* obj        = rc + sizeof(ReferenceCounter);
    //!     // ...
    //!     free(rc); // frees memory of both the counter and the object.
    //! \endcode
    //!
    //! This layout is used for better locality and performance: it groups two allocations into one.
    //! The internal reference counting system also supports copying shared pointers using their raw pointers:
    //! \code{.cpp}
    //!     RefCountPtr<MyObject> ptr1 = MakeShared<MyObject>(); // refcount = 1
    //!     RefCountPtr<MyObject> ptr2 = ptr1;                   // refcount = 2 <-- Valid for std::shared_ptr too
    //!     RefCountPtr<MyObject> ptr3 = ptr1.GetRaw();          // refcount = 3 <-- Also valid here!
    //! \endcode
    //!
    //! It also has some limitations: the objects must inherit from \ref IObject and all allocations must be
    //! done through use of helper functions \ref MakeShared and \ref AllocateShared. It also means that the current
    //! implementation doesn't support weak references as there's no way to deallocate the object but keep the
    //! reference counter.
    class ReferenceCounter final
    {
        AtomicInt32 m_StrongRefCount;
        mutable IAllocator* m_Allocator;

    public:
        FE_STRUCT_RTTI(ReferenceCounter, "32BB0481-9163-4E1A-AB7B-A9B6017D9C8D");

        //! \brief Create a new reference counter with specified allocator.
        //!
        //! The specified allocator will be used to free memory after the counter reaches zero.
        //! This constructor initilizes the counter to _zero_.
        //!
        //! \param [in] allocator - The allocator to use to free memory.
        inline ReferenceCounter(IAllocator* allocator)
            : m_StrongRefCount(0)
            , m_Allocator(allocator)
        {
        }

        //! \brief Add a strong reference to the counter.
        inline uint32_t AddStrongRef()
        {
            return Interlocked::Increment(m_StrongRefCount);
        }

        //! \brief Remove a strong reference from the counter.
        //!
        //! This function will delete the counter itself if number of references reaches zero.
        //!
        //! \param [in] destroyCallback - A function to invoke _after_ deallocation if the counter reached zero.
        //!                               This is typically a lambda that calls object destructor.
        //! \tparam F                   - Type of callback function.
        //!
        //! \return The new (decremented) number of strong references.
        template<class F>
        inline uint32_t ReleaseStrongRef(F&& destroyCallback)
        {
            if (Interlocked::Decrement(m_StrongRefCount) == 0)
            {
                destroyCallback();
                m_Allocator->Deallocate(this, FE_SRCPOS());
            }

            return m_StrongRefCount;
        }
    };
} // namespace FE
