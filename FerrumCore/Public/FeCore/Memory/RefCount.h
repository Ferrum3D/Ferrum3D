#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/RTTI/RTTI.h>
#include <memory_resource>

namespace FE
{
    namespace Internal
    {
        class RcBase;
    }


    namespace Memory
    {
        struct RefCountedObjectBase
        {
            FE_RTTI_Class(RefCountedObjectBase, "B4FA5C63-69C0-4666-8A92-726F070D769B");

            RefCountedObjectBase() = default;
            virtual ~RefCountedObjectBase() = default;

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

        private:
            friend class ::FE::Internal::RcBase;

            RefCountedObjectBase(const RefCountedObjectBase&) = delete;
            RefCountedObjectBase& operator=(const RefCountedObjectBase&) = delete;

            RefCountedObjectBase(RefCountedObjectBase&&) = delete;
            RefCountedObjectBase& operator=(RefCountedObjectBase&&) = delete;

        protected:
            std::atomic<uint32_t> m_refCount = 0;
            uint32_t m_allocationSize = 0;
            std::pmr::memory_resource* m_allocator = nullptr;

            virtual void DoRelease()
            {
                std::pmr::memory_resource* pAllocator = m_allocator;
                const size_t allocationSize = m_allocationSize;
                this->~RefCountedObjectBase();
                pAllocator->deallocate(this, allocationSize);
            }
        };
    } // namespace Memory


    namespace Internal
    {
        class RcBase
        {
        protected:
            static void SetupRefCounter(Memory::RefCountedObjectBase* pObject, std::pmr::memory_resource* pAllocator,
                                        uint32_t allocationSize)
            {
                pObject->m_allocationSize = allocationSize;
                pObject->m_allocator = pAllocator;
            }
        };
    } // namespace Internal


    template<class T>
    struct Rc final : public Internal::RcBase
    {
        using ValueType = T;

        Rc() = default;

        ~Rc()
        {
            InternalRelease();
        }

        Rc(T* pObject)
            : m_object(pObject)
        {
            InternalAddRef();
        }

        Rc(const Rc& other)
            : m_object(other.Get())
        {
            InternalAddRef();
        }

        template<class TOther>
        Rc(const Rc<TOther>& other)
            : m_object(other.Get())
        {
            InternalAddRef();
        }

        Rc(Rc&& other)
            : m_object(other.Detach())
        {
        }

        template<class TOther>
        Rc(Rc<TOther>&& other)
            : m_object(other.Detach())
        {
        }

        Rc& operator=(const Rc& other)
        {
            if (static_cast<Internal::RcBase*>(this) == static_cast<const Internal::RcBase*>(&other))
                return *this;

            Attach(other.Get());
            InternalAddRef();
            return *this;
        }

        template<class TOther>
        Rc& operator=(const Rc<TOther>& other)
        {
            if (static_cast<Internal::RcBase*>(this) == static_cast<const Internal::RcBase*>(&other))
                return *this;

            Attach(other.Get());
            InternalAddRef();
            return *this;
        }

        Rc& operator=(Rc&& other)
        {
            Attach(other.Detach());
            return *this;
        }

        template<class TOther>
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
        //!
        //! It is the same as using unary '&' operator.
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

        template<class... TArgs>
        static T* New(std::pmr::memory_resource* pAllocator, TArgs&&... args)
        {
            T* ptr = new (pAllocator->allocate(sizeof(T), Memory::kDefaultAlignment)) T(std::forward<TArgs>(args)...);
            if constexpr (std::is_base_of_v<Memory::RefCountedObjectBase, T>)
                SetupRefCounter(ptr, pAllocator, static_cast<uint32_t>(sizeof(T)));
            return ptr;
        }

        template<class... TArgs>
        static T* DefaultNew(TArgs&&... args)
        {
            std::pmr::memory_resource* pAllocator = std::pmr::get_default_resource();
            T* ptr = new (pAllocator->allocate(sizeof(T), Memory::kDefaultAlignment)) T(std::forward<TArgs>(args)...);
            if constexpr (std::is_base_of_v<Memory::RefCountedObjectBase, T>)
                SetupRefCounter(ptr, pAllocator, static_cast<uint32_t>(sizeof(T)));
            return ptr;
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
    inline bool operator==(const Rc<T>& lhs, std::nullptr_t)
    {
        return lhs.Get() == nullptr;
    }


    template<class T>
    inline bool operator!=(const Rc<T>& lhs, std::nullptr_t)
    {
        return lhs.Get() != nullptr;
    }


    template<class T>
    inline bool operator==(std::nullptr_t, const Rc<T>& rhs)
    {
        return rhs.Get() == nullptr;
    }


    template<class T>
    inline bool operator!=(std::nullptr_t, const Rc<T>& rhs)
    {
        return rhs.Get() != nullptr;
    }


    template<class T1, class T2>
    inline bool operator==(const Rc<T1>& lhs, T2* rhs)
    {
        return lhs.Get() == rhs;
    }


    template<class T1, class T2>
    inline bool operator!=(const Rc<T1>& lhs, T2* rhs)
    {
        return lhs.Get() != rhs;
    }


    template<class T1, class T2>
    inline bool operator==(T1* lhs, const Rc<T2>& rhs)
    {
        return rhs == lhs.Get();
    }


    template<class T1, class T2>
    inline bool operator!=(T1* lhs, const Rc<T2>& rhs)
    {
        return lhs != rhs.Get();
    }


    template<class T1, class T2>
    inline bool operator==(const Rc<T1>& lhs, const Rc<T2>& rhs)
    {
        return lhs.Get() == rhs.Get();
    }


    template<class T1, class T2>
    inline bool operator!=(const Rc<T1>& lhs, const Rc<T2>& rhs)
    {
        return lhs.Get() != rhs.Get();
    }
} // namespace FE
