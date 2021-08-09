#pragma once
#include <FeCore/Memory/ReferenceCounter.h>

namespace FE
{
    //! \brief Basic interface for all reference counted objects in the engine.
    //!
    //! This interface is required for classes to use them in ref-counted pointers.
    class IObject
    {
    public:
        FE_CLASS_RTTI(IObject, "B4FA5C63-69C0-4666-8A92-726F070D769B");

        virtual ~IObject() = default;

        //! \brief Add a strong reference to object's reference counter.
        //!
        //! \return The new (incremented) value of strong reference counter.
        virtual UInt32 AddStrongRef() = 0;

        //! \brief Remove a strong reference from object's reference counter.
        //! 
        //! If reference counter reaches zero the object will commit suicide by deallocating
        //! it's own memory and calling it's own destructor.
        //!
        //! \return The new (decremented) value of strong reference counter.
        virtual UInt32 ReleaseStrongRef() = 0;

        //! \brief Attach a \ref ReferenceCounter to this object.
        //!
        //! This function must be called right after the object was allocated. It must provide the pointer to
        //! reference counter created exclusively for this object.
        //!
        //! \note It's not recommended to use this function directly as there're
        //!       helpers called \ref MakeShared and \ref AllocateShared that do this automatically.
        //!
        //! \param [in] counter - The reference counter to attach to this object.
        virtual void AttachRefCounter(ReferenceCounter* counter) = 0;

        //! \brief Get reference counter that belongs to this object.
        //!
        //! \return The attached reference counter.
        virtual ReferenceCounter* GetRefCounter() = 0;
    };

    //! \brief Base class for dynamic managed objects in the engine.
    //!
    //! Example:
    //! \code{.cpp}
    //!     class IObject {};
    //!     class IConsoleLogger : public IObject {};
    //!     class ConsoleLogger  : public Object<IConsoleLogger> {};
    //! \endcode
    //!
    //! \tparam TInterface - The type of interface the object must implement.
    template<class TInterface, std::enable_if_t<std::is_base_of_v<IObject, TInterface>, bool> = true>
    class Object : public TInterface
    {
        ReferenceCounter* m_RefCounter = nullptr;

    public:
        FE_CLASS_RTTI(Object, "97E22B50-3B0D-4B70-A1DF-6665F6EF72A3");

        virtual ~Object() = default;

        inline UInt32 AddStrongRef() override
        {
            return m_RefCounter->AddStrongRef();
        }

        //! Directly uses \ref ReferenceCounter::ReleaseStrongRef, but also calls the virtual desctructor
        //! of itself (commits suicide) when the reference counter reaches zero.
        inline UInt32 ReleaseStrongRef() override
        {
            return m_RefCounter->ReleaseStrongRef([this]() {
                this->~Object();
            });
        }

        inline void AttachRefCounter(ReferenceCounter* refCounter) override
        {
            m_RefCounter = refCounter;
        }

        inline ReferenceCounter* GetRefCounter() override
        {
            return m_RefCounter;
        }
    };
} // namespace FE
