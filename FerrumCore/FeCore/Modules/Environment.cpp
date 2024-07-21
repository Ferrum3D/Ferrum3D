#include <FeCore/Console/Console.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Utils/SortedStringVector.h>
#include <cstdio>
#include <sstream>

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
} // namespace FE

namespace FE::Env
{
    namespace Internal
    {
        class DefaultMemoryResource final : public std::pmr::memory_resource
        {
        public:
            inline void* do_allocate(size_t size, size_t alignment) override
            {
                return Memory::DefaultAllocate(size, alignment);
            }

            inline void do_deallocate(void* p, size_t, size_t) override
            {
                Memory::DefaultFree(p);
            }

            inline bool do_is_equal(const memory_resource&) const noexcept override
            {
                return false;
            }
        };

        static IEnvironment* g_EnvInstance = nullptr;
        static bool g_IsEnvOwner = false;

        class Environment : public IEnvironment
        {
            using Map = SortedStringVector<void*, std::vector<char>, std::string_view>;

            Map m_Map;
            Mutex m_Lock;
            DefaultMemoryResource m_DefaultMemoryResource;

            inline VariableResult FindVariableNoLock(std::string_view name)
            {
                auto it = m_Map.FindCIter(name);
                if (it != m_Map.end())
                {
                    auto& vec = std::get<0>(*it);
                    auto view = std::string_view(vec.data(), vec.size());
                    if (view == name)
                    {
                        return std::make_tuple(std::get<1>(*it), VariableOk::Found);
                    }
                }

                return Err(VariableError::NotFound);
            }

        public:
            inline Environment()
            {
                std::pmr::set_default_resource(&m_DefaultMemoryResource);

                Console::Init();
                Console::SetColor(Console::Color::Green);
                // std::stringstream ss;
                // ss << "====================[ Ferrum v" << FerrumVersion.Major << "." << FerrumVersion.Minor;
                // ss << " Global environment created ]====================\n";
                // puts(ss.str().c_str());
                Console::ResetColor();
            }

            inline std::pmr::memory_resource* GetDefaultMemoryResource()
            {
                return &m_DefaultMemoryResource;
            }

            inline VariableResult FindVariable(std::string_view name) override
            {
                std::unique_lock lk(m_Lock);
                return FindVariableNoLock(name);
            }

            inline VariableResult CreateVariable(std::vector<char>&& name, size_t size, size_t alignment,
                                                 std::string_view& nameView) override
            {
                std::unique_lock lk(m_Lock);
                auto find = FindVariableNoLock(std::string_view(name.data(), name.size()));
                if (find.IsOk())
                {
                    return find;
                }

                void* storage = Memory::DefaultAllocate(size, alignment);
                if (!storage)
                    return VariableResult::Err(VariableError::AllocationError);

                auto& [str, ptr] = m_Map.Push(std::move(name), storage);
                nameView = std::string_view(str.data(), str.size());
                return std::make_tuple(storage, VariableOk::Created);
            }

            inline VariableResult RemoveVariable(std::string_view name) override
            {
                std::unique_lock lk(m_Lock);
                auto it = m_Map.FindIter(name);
                if (it == m_Map.end())
                {
                    return Err(VariableError::NotFound);
                }

                auto result = std::make_tuple(std::get<1>(*it), VariableOk::Removed);

                // Don't erase the variable as std::vector<...>.erase() is a very expensive operation (requires
                // moving half the vector to the left).
                // That's why we just place a nullptr instead of the previous pointer, so we know the variable doesn't exist anymore.
                // The handle (string + pointer) is quite light-weight and we don't use very many global variables and singletons,
                // maybe not more than a hundred, so it would be reasonable to just leak them for some performance benefits.
                // TODO: it's possible to implement some garbage collection here and periodically remove all nullptr-variables from the map.
                std::get<1>(*it) = nullptr;
                return result;
            }

            inline void Destroy() override
            {
                Console::SetColor(Console::Color::Green);
                // puts("\n\n====================[ Ferrum3D Global environment destroyed ]=====================");
                Console::ResetColor();
                int leaked = 0;
                for (auto& var : m_Map)
                {
                    if (std::get<1>(var))
                    {
                        ++leaked;
                    }
                }

#if FE_DEBUG
                if (leaked)
                {
                    printf("Variables leaked: %i, destroying them manually...\n", leaked);
                    Console::SetColor(Console::Color::Red);
                    puts("Destructors of leaked variables won't be called!");
                    puts("Be sure to free all global variables before global environment shut-down");
                    Console::ResetColor();
                    for (auto& var : m_Map)
                    {
                        if (std::get<1>(var))
                        {
                            printf("Freeing variable \"%s\"...\n", std::get<0>(var).data());
                            Memory::DefaultFree(std::get<1>(var));
                        }
                    }
                }
#endif

                Memory::DefaultFree(this);
            }
        };
    } // namespace Internal

    void CreateEnvironment()
    {
        if (Internal::g_EnvInstance)
        {
            return;
        }

        Internal::g_IsEnvOwner = true;
        Internal::g_EnvInstance = Memory::DefaultNew<Internal::Environment>();
    }

    Internal::IEnvironment& GetEnvironment()
    {
        FE_CORE_ASSERT(Internal::g_EnvInstance, "Environment must be created explicitly");
        return *Internal::g_EnvInstance;
    }

    void AttachEnvironment(Internal::IEnvironment& instance)
    {
        DetachEnvironment();

        Internal::g_EnvInstance = &instance;
        std::pmr::set_default_resource(static_cast<Internal::Environment&>(instance).GetDefaultMemoryResource());
    }

    void DetachEnvironment()
    {
        if (!Internal::g_EnvInstance)
        {
            return;
        }

        if (Internal::g_IsEnvOwner)
        {
            GetEnvironment().Destroy();
            Internal::g_EnvInstance = nullptr;
        }
    }

    [[maybe_unused]] struct EnvironmentRelease
    {
        ~EnvironmentRelease()
        {
            DetachEnvironment();
        }
    } g_EnvRelease;

    bool EnvironmentAttached()
    {
        return Internal::g_EnvInstance;
    }
} // namespace FE::Env
