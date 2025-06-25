#pragma once
#include <FeCore/DI/BaseDI.h>
#include <FeCore/Memory/RefCount.h>
#include <FeCore/Modules/Environment.h>
#include <festd/fixed_function.h>

namespace FE::DI
{
    using ActivatorFunction = festd::fixed_function<48, ResultCode(IServiceProvider*, Memory::RefCountedObjectBase**)>;


    struct alignas(64) ServiceActivator final
    {
        ResultCode Invoke(IServiceProvider* pServiceProvider, Memory::RefCountedObjectBase** result) const
        {
            FE_PROFILER_ZONE();
            return m_function(pServiceProvider, result);
        }

        static ServiceActivator CreateFromFunction(ActivatorFunction&& func)
        {
            ServiceActivator result;
            result.m_function = std::move(func);
            return result;
        }

        template<class T, class = std::enable_if_t<std::is_base_of_v<Memory::RefCountedObjectBase, T>>>
        static ServiceActivator CreateForType()
        {
            ServiceActivator activator{};
            activator.m_function = [](IServiceProvider* provider, Memory::RefCountedObjectBase** result) {
                ResolveContext ctx{ std::pmr::get_default_resource(), provider };

                FE_Assert(result, "result was null");
                *result = ctx.CreateService<T>();
                return ctx.m_resolveResult;
            };

            return activator;
        }

        template<class T>
        static festd::expected<T*, ResultCode> Instantiate(IServiceProvider* provider, std::pmr::memory_resource* pAllocator)
        {
            ResolveContext ctx{ pAllocator, provider };

            T* pObject = ctx.CreateService<T>();
            if (ctx.m_resolveResult != ResultCode::kSuccess)
                return festd::unexpected(ctx.m_resolveResult);

            return pObject;
        }

    private:
        ActivatorFunction m_function{};

        template<class T>
        using RemovePtr = std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<T>>>;

        struct ResolveContext final
        {
            std::pmr::memory_resource* m_allocator = nullptr;
            IServiceProvider* m_serviceProvider = nullptr;
            ResultCode m_resolveResult = ResultCode::kSuccess;

            ResolveContext(std::pmr::memory_resource* pAllocator, IServiceProvider* provider)
                : m_allocator(pAllocator)
                , m_serviceProvider(provider)
            {
            }

            struct ArgResolver final
            {
                ResolveContext* m_context;

                FE_FORCE_INLINE explicit constexpr ArgResolver(ResolveContext* ctx)
                    : m_context(ctx)
                {
                }

                template<class T, class = std::enable_if_t<std::is_pointer_v<T>>>
                FE_FORCE_INLINE operator T()
                {
                    return m_context->GetService<RemovePtr<T>>();
                }
            };

            template<class T>
            T* GetService()
            {
                if (m_resolveResult != ResultCode::kSuccess)
                    return nullptr;

                const festd::expected<T*, ResultCode> result = m_serviceProvider->Resolve<T>();
                if (result)
                    return result.value();

                m_resolveResult = result.error();
                return nullptr;
            }

            template<class T, class... TArgs>
            FE_FORCE_INLINE std::enable_if_t<std::is_base_of_v<Memory::RefCountedObjectBase, T>, T*> NewImpl(TArgs&&... args)
            {
                return Rc<T>::New(m_allocator, std::forward<TArgs>(args)...);
            }

            template<class T, class... TArgs>
            FE_FORCE_INLINE std::enable_if_t<!std::is_base_of_v<Memory::RefCountedObjectBase, T>, T*> NewImpl(TArgs&&... args)
            {
                return Memory::New<T>(m_allocator, std::forward<TArgs>(args)...);
            }

            template<class T, class TArg>
            FE_FORCE_INLINE std::enable_if_t<!std::is_constructible_v<T, TArg>, T*> CreateServiceImpl(TArg)
            {
                return NewImpl<T>();
            }

            template<class T, class TArg>
            FE_FORCE_INLINE std::enable_if_t<std::is_constructible_v<T, TArg>, T*> CreateServiceImpl(TArg a)
            {
                return NewImpl<T>(a);
            }

            template<class T, class TArg1, class TArg2, class... TArgs>
            FE_FORCE_INLINE std::enable_if_t<std::is_constructible_v<T, TArg1, TArg2, TArgs...>, T*> //
            CreateServiceImpl(TArg1 a1, TArg2 a2, TArgs... args)
            {
                return NewImpl<T>(a1, a2, args...);
            }

            template<class T, class TArg1, class TArg2, class... TArgs>
            FE_FORCE_INLINE std::enable_if_t<!std::is_constructible_v<T, TArg1, TArg2, TArgs...>, T*> //
            CreateServiceImpl(TArg1, TArg2 a2, TArgs... args)
            {
                return CreateServiceImpl<T>(a2, args...);
            }

            template<class T>
            FE_FORCE_INLINE T* CreateService()
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
    };


    template<class T>
    FE_FORCE_INLINE festd::expected<T*, ResultCode> New(std::pmr::memory_resource* allocator)
    {
        //
        // TODO:
        //   - It might be dangerous to use the provided allocator to instantiate the dependencies
        //     as that allocator is only required to live as long as the created object is alive,
        //     while the dependencies can be shared and therefore live longer.
        //   - The initial problem will probably be solved in the future when our RTTI (or DI) system
        //     will be capable of storing class-specific allocators. For instance, such a system
        //     would allow us to create memory pools for certain classes like `Buffer` and `Image` easily.
        //

        return ServiceActivator::Instantiate<T>(Env::GetServiceProvider(), allocator);
    }


    template<class T>
    FE_FORCE_INLINE festd::expected<T*, ResultCode> DefaultNew()
    {
        return ServiceActivator::Instantiate<T>(Env::GetServiceProvider(), std::pmr::get_default_resource());
    }
} // namespace FE::DI
