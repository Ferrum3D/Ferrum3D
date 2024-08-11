#pragma once
#include <FeCore/DI/BaseDI.h>
#include <FeCore/Memory/RefCount.h>

namespace FE::DI
{
    using ActivatorFunction = festd::fixed_function<48, ResultCode(IServiceProvider*, Memory::RefCountedObjectBase**)>;


    class alignas(64) ServiceActivator final
    {
        ActivatorFunction m_Function;


        template<class T>
        using RemovePtr = std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<T>>>;


        struct ResolveContext final
        {
            std::pmr::memory_resource* pAllocator = nullptr;
            IServiceProvider* pServiceProvider = nullptr;
            ResultCode ResolveResult = ResultCode::Success;

            inline ResolveContext(std::pmr::memory_resource* pAllocator, IServiceProvider* pProvider)
                : pAllocator(pAllocator)
                , pServiceProvider(pProvider)
            {
            }

            struct ArgResolver final
            {
                ResolveContext* pContext;

                FE_FORCE_INLINE explicit constexpr ArgResolver(ResolveContext* ctx)
                    : pContext(ctx)
                {
                }

                template<class T, class = std::enable_if_t<std::is_pointer_v<T>>>
                FE_FORCE_INLINE operator T()
                {
                    return pContext->GetService<RemovePtr<T>>();
                }
            };

            template<class T>
            inline T* GetService()
            {
                if (ResolveResult != ResultCode::Success)
                    return nullptr;

                const Result<T*, ResultCode> result = pServiceProvider->Resolve<T>();
                if (result)
                    return result.Unwrap();

                ResolveResult = result.UnwrapErr();
                return nullptr;
            }

            template<class T, class TArg>
            inline std::enable_if_t<!std::is_constructible_v<T, TArg>, T*> CreateServiceImpl(TArg)
            {
                return Rc<T>::New(pAllocator);
            }

            template<class T, class TArg>
            inline std::enable_if_t<std::is_constructible_v<T, TArg>, T*> CreateServiceImpl(TArg a)
            {
                return Rc<T>::New(pAllocator, a);
            }

            template<class T, class TArg1, class TArg2, class... TArgs>
            inline std::enable_if_t<std::is_constructible_v<T, TArg1, TArg2, TArgs...>, T*> //
            CreateServiceImpl(TArg1 a1, TArg2 a2, TArgs... args)
            {
                return Rc<T>::New(pAllocator, a1, a2, args...);
            }

            template<class T, class TArg1, class TArg2, class... TArgs>
            inline std::enable_if_t<!std::is_constructible_v<T, TArg1, TArg2, TArgs...>, T*> //
            CreateServiceImpl(TArg1, TArg2 a2, TArgs... args)
            {
                return CreateServiceImpl<T>(a2, args...);
            }

            template<class T>
            inline T* CreateService()
            {
                return CreateServiceImpl<T>(ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this),
                                            ArgResolver(this));
            }
        };

    public:
        inline ResultCode Invoke(IServiceProvider* pServiceProvider, Memory::RefCountedObjectBase** ppResult) const
        {
            ZoneScoped;
            return m_Function(pServiceProvider, ppResult);
        }

        inline static ServiceActivator CreateFromFunction(ActivatorFunction&& func)
        {
            ServiceActivator result;
            result.m_Function = std::move(func);
            return result;
        }

        template<class T>
        inline static ServiceActivator CreateForType()
        {
            ServiceActivator result{};
            result.m_Function = [](IServiceProvider* pProvider, Memory::RefCountedObjectBase** ppResult) {
                ResolveContext ctx{ std::pmr::get_default_resource(), pProvider };

                FE_CORE_ASSERT(ppResult, "ppResult was null");
                *ppResult = ctx.CreateService<T>();
                return ctx.ResolveResult;
            };

            return result;
        }
    };
} // namespace FE::DI
