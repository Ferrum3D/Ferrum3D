#pragma once
#include <FeCore/Memory/ReferenceCounter.h>

namespace FE
{
    /**
     * @brief Basic interface for all reference counted objects in the engine.
     * 
     * This interface is required for classes to use them in ref-counted pointers.
    */
    class IObject
    {
    public:
        FE_CLASS_RTTI(IObject, "B4FA5C63-69C0-4666-8A92-726F070D769B");

        virtual ~IObject() = default;

        virtual UInt32 AddStrongRef()                            = 0;
        virtual UInt32 ReleaseStrongRef()                        = 0;
        virtual void AttachRefCounter(ReferenceCounter* counter) = 0;
        virtual ReferenceCounter* GetRefCounter()                = 0;
    };

    /**
     * Example:
     *     class IObject {};
     *     class IConsoleLogger : public IObject {};
     *     class ConsoleLogger  : public Object<IConsoleLogger> {};
     */
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
