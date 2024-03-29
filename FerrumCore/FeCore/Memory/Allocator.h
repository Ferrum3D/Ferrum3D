#pragma once
#include <FeCore/Memory/AllocatorBase.h>
#include <FeCore/Memory/IAllocator.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE
{
    //! \brief Global allocator holder.
    //!
    //! This class works just like \ref FE::ServiceLocator<T>, but keeps the entire allocator
    //! object in a \ref Env::GlobalVariable<T> and has some allocator-specific functions.
    //!
    //! TAlloc must derive from \ref FE::AllocatorBase.
    //!
    //! \tparam TAlloc - Type of allocator.
    template<class TAlloc>
    class GlobalAllocator
    {
        static Env::GlobalVariable<TAlloc> m_Instance;

        inline static void TryFind()
        {
            Env::FindGlobalVariableByType<TAlloc>().AndThen([&](auto variable) {
                m_Instance = variable;
                return variable;
            });
        }

    public:
        FE_CLASS_RTTI(GlobalAllocator<TAlloc>, "1EF91D3D-58ED-4E4E-8AAC-206072D356CE");

        //! \brief Get instance of allocator. Must be already created.
        //! \return Reference to allocator instance.
        inline static IAllocator& Get()
        {
            if (!m_Instance)
                TryFind();
            FE_CORE_ASSERT(m_Instance, "Allocator must be created before first use");
            return *m_Instance;
        }

        //! \brief Get allocator info. Must be already created.
        //! \return Reference to allocator info.
        inline static IAllocatorInfo& GetInfo()
        {
            if (!m_Instance)
                TryFind();
            FE_CORE_ASSERT(m_Instance, "Allocator must be created before first use");
            return *m_Instance;
        }

        //! \brief Create an instance of allocator and call it's `Init()` member function.
        //!
        //! This function will try to find instance of this allocator if it doesn't exist in current module.
        //! If the allocator is not found anywhere in global environment, it will be created and initialized.
        //!
        //! \param [in] desc - Allocator configuration struct.
        inline static void Init(const typename TAlloc::Desc& desc)
        {
            if (!m_Instance)
                m_Instance = Env::CreateGlobalVariableByType<TAlloc>();
            if (!m_Instance->Initialized())
                m_Instance->Init(desc);
        }

        inline static void ThreadInit()
        {
            m_Instance->ThreadInit();
        }

        //! \brief Remove reference to the allocator from this module.
        inline static void Destroy()
        {
            if (!m_Instance)
                TryFind();
            FE_CORE_ASSERT(m_Instance, "Allocator must be created before first use");
            m_Instance.Reset();
        }

        //! \brief Check if the allocator is created and ready to use.
        //! 
        //! \return True if allocator is initialized.
        inline static bool Initialized()
        {
            if (!m_Instance)
                TryFind();
            return m_Instance && m_Instance->Initialized();
        }
    };

    template<class TAlloc>
    Env::GlobalVariable<TAlloc> GlobalAllocator<TAlloc>::m_Instance;
} // namespace FE
