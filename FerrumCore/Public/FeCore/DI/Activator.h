#pragma once
#include <FeCore/DI/BaseDI.h>
#include <FeCore/Memory/RefCount.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/RTTI/Reflection.h>
#include <festd/fixed_function.h>

namespace FE::DI
{
    using ActivatorFunction = festd::fixed_function<48, ActivatorFunctionType>;


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
            ServiceActivator activator;
            activator.m_function = Rtti::GetType<T>().m_activator;
            FE_Assert(activator.m_function, "Specified type does not support DI");
            return activator;
        }

        template<class T>
        static festd::expected<T*, ResultCode> Instantiate(IServiceProvider* provider, std::pmr::memory_resource* pAllocator)
        {
            FE_Unused(pAllocator); // TODO

            Memory::RefCountedObjectBase* object;
            const ResultCode result = Rtti::GetType<T>().m_activator(provider, &object);
            if (result != ResultCode::kSuccess)
                return festd::unexpected(result);

            return Rtti::AssertCast<T*>(object);
        }

    private:
        ActivatorFunction m_function{};
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
