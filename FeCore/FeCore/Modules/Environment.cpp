#include "Environment.h"
#include <Memory/BasicSystemAllocator.h>
#include <Utils/SortedStringVector.h>

namespace FE
{
    template<>
    struct StringToView<std::vector<char>, std::string_view>
    {
        inline std::string_view operator()(const std::vector<char>& str)
        {
            return std::string_view(str.data(), str.size());
        }
    };
}

namespace FE::Env
{
    namespace Internal
    {
        static IEnvironment* g_EnvInstance = nullptr;
        static bool g_IsEnvOwner           = false;

        class Environment : public IEnvironment
        {
            using Allocator = StdBasicAllocator<std::tuple<std::vector<char>, void*>>;
            using Map       = SortedStringVector<void*, std::vector<char>, std::string_view, Allocator>;

            FE::IBasicAllocator* m_Allocator = nullptr;
            Map m_Map;
            std::mutex m_Lock;

            inline VariableResult FindVariableNoLock(std::string_view name)
            {
                auto it = m_Map.FindCIter(name);
                if (it != m_Map.end())
                {
                    auto& vec = std::get<0>(*it);
                    auto view = std::string_view(vec.data(), vec.size());
                    if (view == name)
                        return VariableResult::Ok(std::get<1>(*it), VariableOk::Found);
                }

                return VariableResult::Err(VariableError::NotFound);
            }

        public:
            inline Environment(FE::IBasicAllocator* allocator)
                : m_Allocator(allocator)
                , m_Map(Allocator(allocator))
            {
            }

            inline VariableResult FindVariable(std::string_view name) override
            {
                std::unique_lock lk(m_Lock);
                return FindVariableNoLock(name);
            }

            inline VariableResult CreateVariable(
                std::vector<char>&& name, size_t size, size_t alignment, std::string_view& nameView) override
            {
                std::unique_lock lk(m_Lock);
                auto find = FindVariableNoLock(std::string_view(name.data(), name.size()));
                if (find.IsOk())
                    return find;

                void* storage = m_Allocator->Allocate(size, alignment);
                if (!storage)
                    return VariableResult::Err(VariableError::AllocationError);

                auto& [str, ptr] = m_Map.Push(std::move(name), std::move(storage));
                nameView         = std::string_view(str.data(), str.size());
                return VariableResult::Ok(storage, VariableOk::Created);
            }

            inline VariableResult RemoveVariable(std::string_view name) override
            {
                std::unique_lock lk(m_Lock);
                auto it = m_Map.FindIter(name);
                if (it == m_Map.end())
                    return VariableResult::Err(VariableError::NotFound);

                auto result = VariableResult::Ok(std::get<1>(*it), VariableOk::Removed);

                // Don't erase the variable as std::vector<...>.erase() is a very expensive operation (requires
                // moving half the vector to the left).
                // That's why we just place a nullptr instead of the previous pointer, so we know the variable doesn't exist anymore.
                // The handle (string + pointer) is quite light-weight and we don't use very many global variables and singletons,
                // maybe not more than a hundred, so it would be resonable to just leak them for some performance benefits.
                // TODO: it's possible to implement some garbage collection here and periodically remove all nullptr-variables
                // from the map.
                std::get<1>(*it) = nullptr;
                return result;
            }

            inline FE::IBasicAllocator* GetAllocator() override
            {
                return m_Allocator;
            }

            inline void Destroy() override
            {
                m_Allocator->Deallocate(this);
            }
        };
    } // namespace Internal

    void CreateEnvironment(IBasicAllocator* allocator)
    {
        if (Internal::g_EnvInstance)
            return;

        if (!allocator)
        {
            static BasicSystemAllocator sysAlloc;
            allocator = &sysAlloc;
        }

        Internal::g_EnvInstance = new (allocator->Allocate(sizeof(Internal::Environment), alignof(Internal::Environment)))
            Internal::Environment(allocator);
    }

    Internal::IEnvironment& GetEnvironment()
    {
        if (!Internal::g_EnvInstance)
        {
            FE_CORE_ASSERT(Internal::g_EnvInstance, "Environment must be created explicitly");
            // CreateEnvironment(nullptr);
        }
        return *Internal::g_EnvInstance;
    }

    void AttachEnvironment(Internal::IEnvironment& instance)
    {
        DetachEnvironment();

        Internal::g_EnvInstance = &instance;
    }

    void DetachEnvironment()
    {
        if (!Internal::g_EnvInstance)
            return;

        if (Internal::g_IsEnvOwner)
            GetEnvironment().Destroy();

        Internal::g_EnvInstance = nullptr;
    }

    bool EnvironmentAttached()
    {
        return Internal::g_EnvInstance;
    }
} // namespace FE::Env
