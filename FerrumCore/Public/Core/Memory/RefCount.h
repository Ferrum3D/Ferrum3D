#pragma once
#include <Core/Base/BaseTypes.h>
#include <Core/RTTI/RTTI.h>
#include <atomic>
#include <memory_resource>

namespace FE
{
    namespace Memory
    {
        struct RefCountedObjectBase
        {
            FE_RTTI("B4FA5C63-69C0-4666-8A92-726F070D769B");

            RefCountedObjectBase() = default;
            virtual ~RefCountedObjectBase() = default;

            RefCountedObjectBase(const RefCountedObjectBase&) = delete;
            RefCountedObjectBase& operator=(const RefCountedObjectBase&) = delete;

            RefCountedObjectBase(RefCountedObjectBase&&) = delete;
            RefCountedObjectBase& operator=(RefCountedObjectBase&&) = delete;

            [[nodiscard]] uint32_t GetRefCount() const
            {
                return m_refCount.load(std::memory_order_relaxed);
            }

            uint32_t AddRef()
            {
                return ++m_refCount;
            }

            uint32_t Release()
            {
                const uint32_t refCount = --m_refCount;
                if (refCount == 0)
                    DoRelease();

                return refCount;
            }

        protected:
            virtual void DoRelease() = 0;

        private:
            std::atomic<uint32_t> m_refCount = 0;
        };
    } // namespace Memory


    template<class T>
    struct Rc final
    {
        using ValueType = T;

        Rc() = default;

        ~Rc()
        {
            InternalRelease();
        }

        Rc(T* object)
            : m_object(object)
        {
            InternalAddRef();
        }

        Rc(const Rc& other)
            : m_object(other.Get())
        {
            InternalAddRef();
        }

        template<class TOther>
            requires std::assignable_from<T*&, TOther*>
        Rc(const Rc<TOther>& other)
            : m_object(other.Get())
        {
            InternalAddRef();
        }

        Rc(Rc&& other) noexcept
            : m_object(other.Detach())
        {
        }

        template<class TOther>
            requires std::assignable_from<T*&, TOther*>
        Rc(Rc<TOther>&& other)
            : m_object(other.Detach())
        {
        }

        Rc& operator=(const Rc& other)
        {
            if (this == &other)
                return *this;

            Attach(other.Get());
            InternalAddRef();
            return *this;
        }

        template<class TOther>
            requires std::assignable_from<T*&, TOther*>
        Rc& operator=(const Rc<TOther>& other)
        {
            if (reinterpret_cast<uintptr_t>(this) == reinterpret_cast<uintptr_t>(&other))
                return *this;

            Attach(other.Get());
            InternalAddRef();
            return *this;
        }

        Rc& operator=(Rc&& other) noexcept
        {
            Attach(other.Detach());
            return *this;
        }

        template<class TOther>
            requires std::assignable_from<T*&, TOther*>
        Rc& operator=(Rc<TOther>&& other)
        {
            Attach(other.Detach());
            return *this;
        }

        //! @brief Release a reference and reset to null.
        void Reset()
        {
            InternalRelease();
        }

        //! @brief Get pointer to the stored pointer.
        [[nodiscard]] T* const* GetAddressOf() const
        {
            return &m_object;
        }

        //! @brief Get pointer to the stored pointer.
        [[nodiscard]] T** GetAddressOf()
        {
            return &m_object;
        }

        //! @brief Release a reference and get pointer to the stored pointer.
        [[nodiscard]] T** ReleaseAndGetAddressOf()
        {
            InternalRelease();
            return &m_object;
        }

        //! @brief Attach a pointer and do not add strong reference.
        void Attach(T* pObject)
        {
            InternalRelease();
            m_object = pObject;
        }

        //! @brief Detach the stored pointer: reset to null without decrementing the reference counter.
        T* Detach()
        {
            T* ptr = m_object;
            m_object = nullptr;
            return ptr;
        }

        //! @brief Get underlying raw pointer.
        [[nodiscard]] T* Get() const
        {
            return m_object;
        }

        T& operator*() const
        {
            return *Get();
        }

        T* operator->() const
        {
            return Get();
        }

        explicit operator bool() const
        {
            return Get();
        }

    private:
        T* m_object = nullptr;

        void InternalAddRef() const
        {
            if (m_object)
                m_object->AddRef();
        }

        uint32_t InternalRelease()
        {
            uint32_t result = 0;
            if (m_object)
            {
                result = m_object->Release();
                m_object = nullptr;
            }

            return result;
        }
    };


    template<class T>
    bool operator==(const Rc<T>& lhs, std::nullptr_t)
    {
        return lhs.Get() == nullptr;
    }


    template<class T>
    bool operator!=(const Rc<T>& lhs, std::nullptr_t)
    {
        return lhs.Get() != nullptr;
    }


    template<class T>
    bool operator==(std::nullptr_t, const Rc<T>& rhs)
    {
        return rhs.Get() == nullptr;
    }


    template<class T>
    bool operator!=(std::nullptr_t, const Rc<T>& rhs)
    {
        return rhs.Get() != nullptr;
    }


    template<class T1, class T2>
    bool operator==(const Rc<T1>& lhs, T2* rhs)
    {
        return lhs.Get() == rhs;
    }


    template<class T1, class T2>
    bool operator!=(const Rc<T1>& lhs, T2* rhs)
    {
        return lhs.Get() != rhs;
    }


    template<class T1, class T2>
    bool operator==(T1* lhs, const Rc<T2>& rhs)
    {
        return rhs == lhs.Get();
    }


    template<class T1, class T2>
    bool operator!=(T1* lhs, const Rc<T2>& rhs)
    {
        return lhs != rhs.Get();
    }


    template<class T1, class T2>
    bool operator==(const Rc<T1>& lhs, const Rc<T2>& rhs)
    {
        return lhs.Get() == rhs.Get();
    }


    template<class T1, class T2>
    bool operator!=(const Rc<T1>& lhs, const Rc<T2>& rhs)
    {
        return lhs.Get() != rhs.Get();
    }
} // namespace FE
