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
        inline constexpr size_t DefaultAlignment = 16;
        inline constexpr size_t CacheLineSize = 64;


        class RefCountedObjectBase
        {
            friend class ::FE::Internal::RcBase;

            RefCountedObjectBase(const RefCountedObjectBase&) = delete;
            RefCountedObjectBase& operator=(const RefCountedObjectBase&) = delete;

            RefCountedObjectBase(RefCountedObjectBase&&) = delete;
            RefCountedObjectBase& operator=(RefCountedObjectBase&&) = delete;

            std::atomic<uint32_t> m_RefCount = 0;
            uint32_t m_AllocationSize = 0;
            std::pmr::memory_resource* m_pAllocator = nullptr;

        public:
            FE_RTTI_Class(RefCountedObjectBase, "B4FA5C63-69C0-4666-8A92-726F070D769B");

            RefCountedObjectBase() = default;
            virtual ~RefCountedObjectBase() = default;

            [[nodiscard]] inline uint32_t GetRefCount() const
            {
                return m_RefCount.load(std::memory_order_relaxed);
            }

            inline uint32_t AddRef()
            {
                return ++m_RefCount;
            }

            inline uint32_t Release()
            {
                const uint32_t refCount = --m_RefCount;
                if (refCount == 0)
                {
                    std::pmr::memory_resource* pAllocator = m_pAllocator;
                    const size_t allocationSize = m_AllocationSize;
                    this->~RefCountedObjectBase();
                    pAllocator->deallocate(this, allocationSize, DefaultAlignment);
                }

                return refCount;
            }
        };
    } // namespace Memory


    namespace Internal
    {
        template<class T>
        class PtrRef final
        {
            T* m_Ptr;

        public:
            using ValueType = typename T::ValueType;

            inline explicit PtrRef(T* ptr)
                : m_Ptr(ptr)
            {
            }

            inline operator void**() const
            {
                return reinterpret_cast<void**>(m_Ptr->ReleaseAndGetAddressOf());
            }

            inline operator ValueType**()
            {
                return m_Ptr->ReleaseAndGetAddressOf();
            }

            inline ValueType* operator*()
            {
                return m_Ptr->Get();
            }
        };


        class RcBase
        {
        protected:
            inline static void SetupRefCounter(Memory::RefCountedObjectBase* pObject, std::pmr::memory_resource* pAllocator,
                                               uint32_t allocationSize)
            {
                pObject->m_AllocationSize = allocationSize;
                pObject->m_pAllocator = pAllocator;
            }
        };
    } // namespace Internal


    template<class T>
    class Rc final : public Internal::RcBase
    {
        T* m_pObject = nullptr;

        inline void InternalAddRef() const
        {
            if (m_pObject)
                m_pObject->AddRef();
        }

        inline uint32_t InternalRelease()
        {
            uint32_t result = 0;
            if (m_pObject)
            {
                result = m_pObject->Release();
                m_pObject = nullptr;
            }

            return result;
        }

    public:
        using ValueType = T;

        inline Rc() = default;

        inline ~Rc()
        {
            InternalRelease();
        }

        inline Rc(T* pObject)
            : m_pObject(pObject)
        {
            InternalAddRef();
        }

        inline Rc(const Rc& other)
            : m_pObject(other.Get())
        {
            InternalAddRef();
        }

        template<class TOther>
        inline Rc(const Rc<TOther>& other)
            : m_pObject(other.Get())
        {
            InternalAddRef();
        }

        inline Rc(Rc&& other)
            : m_pObject(other.Detach())
        {
        }

        template<class TOther>
        inline Rc(Rc<TOther>&& other)
            : m_pObject(other.Detach())
        {
        }

        inline Rc& operator=(const Rc& other)
        {
            if (static_cast<Internal::RcBase*>(this) == static_cast<const Internal::RcBase*>(&other))
                return *this;

            Attach(other.Get());
            InternalAddRef();
            return *this;
        }

        template<class TOther>
        inline Rc& operator=(const Rc<TOther>& other)
        {
            if (static_cast<Internal::RcBase*>(this) == static_cast<const Internal::RcBase*>(&other))
                return *this;

            Attach(other.Get());
            InternalAddRef();
            return *this;
        }

        inline Rc& operator=(Rc&& other)
        {
            Attach(other.Detach());
            return *this;
        }

        template<class TOther>
        inline Rc& operator=(Rc<TOther>&& other)
        {
            Attach(other.Detach());
            return *this;
        }

        //! \brief Release a reference and reset to null.
        inline void Reset()
        {
            InternalRelease();
        }

        //! \brief Get pointer to the stored pointer.
        [[nodiscard]] inline T* const* GetAddressOf() const
        {
            return &m_pObject;
        }

        //! \brief Get pointer to the stored pointer.
        [[nodiscard]] inline T** GetAddressOf()
        {
            return &m_pObject;
        }

        //! \brief Release a reference and get pointer to the stored pointer.
        //!
        //! It is the same as using unary '&' operator.
        [[nodiscard]] inline T** ReleaseAndGetAddressOf()
        {
            InternalRelease();
            return &m_pObject;
        }

        //! \brief Attach a pointer and do not add strong reference.
        inline void Attach(T* pObject)
        {
            InternalRelease();
            m_pObject = pObject;
        }

        //! \brief Detach the stored pointer: reset to null without decrementing the reference counter.
        inline T* Detach()
        {
            T* ptr = m_pObject;
            m_pObject = nullptr;
            return ptr;
        }

        //! \brief Get underlying raw pointer.
        [[nodiscard]] inline T* Get() const
        {
            return m_pObject;
        }

        inline Internal::PtrRef<Rc<T>> operator&()
        {
            return Internal::PtrRef<Rc<T>>(this);
        }

        inline T& operator*() const
        {
            return *Get();
        }

        inline T* operator->() const
        {
            return Get();
        }

        inline explicit operator bool() const
        {
            return Get();
        }

        template<class... TArgs>
        inline static T* New(std::pmr::memory_resource* pAllocator, TArgs&&... args)
        {
            T* ptr = new (pAllocator->allocate(sizeof(T), Memory::DefaultAlignment)) T(std::forward<TArgs>(args)...);
            SetupRefCounter(ptr, pAllocator, static_cast<uint32_t>(sizeof(T)));
            return ptr;
        }

        template<class... TArgs>
        inline static T* DefaultNew(TArgs&&... args)
        {
            std::pmr::memory_resource* pAllocator = std::pmr::get_default_resource();
            T* ptr = new (pAllocator->allocate(sizeof(T), Memory::DefaultAlignment)) T(std::forward<TArgs>(args)...);
            SetupRefCounter(ptr, pAllocator, static_cast<uint32_t>(sizeof(T)));
            return ptr;
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
