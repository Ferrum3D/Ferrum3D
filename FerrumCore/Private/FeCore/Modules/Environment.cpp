#include <FeCore/Console/Console.h>
#include <FeCore/DI/Container.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Modules/EnvironmentPrivate.h>
#include <FeCore/Parallel/Thread.h>
#include <FeCore/Platform/Windows/Common.h>
#include <cstdio>
#include <festd/unordered_map.h>
#include <sstream>

namespace FE::Env
{
    bool IsDebuggerPresent()
    {
#if FE_PLATFORM_WINDOWS
        return ::IsDebuggerPresent();
#endif
    }


    namespace Internal
    {
        inline constexpr uint32_t kNamePageShift = 16;
        inline constexpr uint32_t kNamePageByteSize = 1 << kNamePageShift;
        inline constexpr uint32_t kNameBlockShift = 3;
        static_assert(1 << kNameBlockShift == alignof(Name::Record));


        class NameDataAllocator final
        {
            struct NameHandle final
            {
                uint32_t BlockIndex : kNamePageShift - kNameBlockShift;
                uint32_t PageIndex : 32 - kNamePageShift + kNameBlockShift;
            };

            static constexpr uint32_t PageListByteSize = 64 * 1024;
            static constexpr uint32_t MaxPageCount = PageListByteSize / sizeof(void*);

            Threading::SpinLock m_Lock;
            void** m_ppPages;
            uint32_t m_CurrentPageIndex = kInvalidIndex;
            uint32_t m_Offset = kNamePageByteSize;
            festd::unordered_dense_map<uint64_t, uint32_t> m_Map;

        public:
            NameDataAllocator()
            {
                m_ppPages = static_cast<void**>(Memory::AllocateVirtual(PageListByteSize));
            }

            ~NameDataAllocator()
            {
                while (m_CurrentPageIndex != kInvalidIndex)
                {
                    Memory::FreeVirtual(m_ppPages[m_CurrentPageIndex], kNamePageByteSize);
                    --m_CurrentPageIndex;
                }

                Memory::FreeVirtual(m_ppPages, PageListByteSize);
            }

            uint32_t TryFind(uint64_t hash)
            {
                std::lock_guard lk{ m_Lock };
                const auto it = m_Map.find(hash);
                if (it != m_Map.end())
                    return it->second;
                return kInvalidIndex;
            }

            Name::Record* Allocate(uint64_t hash, size_t stringByteSize, uint32_t& handle)
            {
                std::lock_guard lk{ m_Lock };
                const size_t recordHeaderSize = offsetof(Name::Record, m_data);
                const size_t recordSize = AlignUp<1 << kNameBlockShift>(recordHeaderSize + stringByteSize);
                if (recordSize + m_Offset > kNamePageByteSize)
                {
                    m_ppPages[++m_CurrentPageIndex] = Memory::AllocateVirtual(kNamePageByteSize);
                    m_Offset = 0;
                }

                NameHandle result;
                result.PageIndex = m_CurrentPageIndex;
                result.BlockIndex = m_Offset >> kNameBlockShift;
                handle = festd::bit_cast<uint32_t>(result);
                m_Map.insert(std::make_pair(hash, handle));

                void* ptr = static_cast<uint8_t*>(m_ppPages[m_CurrentPageIndex]) + m_Offset;
                m_Offset += static_cast<uint32_t>(recordSize);
                FE_CORE_ASSERT((m_Offset >> kNameBlockShift) << kNameBlockShift == m_Offset, "");
                return static_cast<Name::Record*>(ptr);
            }

            Name::Record* ResolvePointer(uint32_t handleValue) const
            {
                const NameHandle handle = festd::bit_cast<NameHandle>(handleValue);
                const uintptr_t pageAddress = reinterpret_cast<uintptr_t>(m_ppPages[handle.PageIndex]);
                const uintptr_t recordAddress = pageAddress + (static_cast<size_t>(handle.BlockIndex) << kNameBlockShift);
                return reinterpret_cast<Name::Record*>(recordAddress);
            }
        };


        class DefaultMemoryResource final : public std::pmr::memory_resource
        {
        public:
            void* do_allocate(size_t size, size_t alignment) override
            {
                return Memory::DefaultAllocate(size, alignment);
            }

            void do_deallocate(void* p, size_t, size_t) override
            {
                Memory::DefaultFree(p);
            }

            bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        class VirtualMemoryResource final : public std::pmr::memory_resource
        {
        public:
            void* do_allocate(size_t size, size_t alignment) override
            {
                FE_CORE_ASSERT(Memory::GetPlatformSpec().m_granularity >= alignment, "Unsupported alignment");
                return Memory::AllocateVirtual(size);
            }

            void do_deallocate(void* p, size_t size, size_t) override
            {
                return Memory::FreeVirtual(p, size);
            }

            bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        class Environment : public IEnvironment
        {
            struct ProfilerInitScope
            {
                ProfilerInitScope()
                {
                    tracy::StartupProfiler();
                }

                ~ProfilerInitScope()
                {
                    tracy::ShutdownProfiler();
                }
            };

            ProfilerInitScope m_profilerInitScope;
            festd::unordered_dense_map<Name, void*> m_map;
            Mutex m_lock;

            VariableResult FindVariableNoLock(Name name)
            {
                VariableResult result;
                const auto it = m_map.find(name);
                if (it != m_map.end())
                {
                    result.Code = VariableResultCode::Found;
                    result.pData = it->second;
                    return result;
                }

                result.Code = VariableResultCode::NotFound;
                result.pData = nullptr;
                return result;
            }

        public:
            DefaultMemoryResource m_defaultMemoryResource;
            VirtualMemoryResource m_virtualMemoryResource;
            Memory::LockedMemoryResource<Memory::LinearAllocator, Threading::SpinLock> m_linearMemoryResource;

            SharedState m_sharedState;

            NameDataAllocator m_nameDataAllocator;
            DI::Container m_diContainer;

            Environment()
                : m_linearMemoryResource(2 * 1024 * 1024, &m_virtualMemoryResource)
            {
                std::pmr::set_default_resource(&m_defaultMemoryResource);
            }

            std::pmr::memory_resource* GetStaticAllocator(Memory::StaticAllocatorType type)
            {
                switch (type)
                {
                case Memory::StaticAllocatorType::Default:
                    return &m_defaultMemoryResource;
                case Memory::StaticAllocatorType::Virtual:
                    return &m_virtualMemoryResource;
                case Memory::StaticAllocatorType::Linear:
                    return &m_linearMemoryResource;
                default:
                    FE_DebugBreak();
                    return nullptr;
                }
            }

            VariableResult FindVariable(Name name) override
            {
                std::unique_lock lk{ m_lock };
                return FindVariableNoLock(name);
            }

            VariableResult CreateVariable(Name name, size_t size, size_t alignment) override
            {
                std::unique_lock lk{ m_lock };
                VariableResult result = FindVariableNoLock(name);
                if (result.Code == VariableResultCode::Found)
                    return result;

                result.Code = VariableResultCode::Created;
                result.pData = Memory::DefaultAllocate(size, alignment);
                m_map[name] = result.pData;
                return result;
            }

            VariableResult RemoveVariable(Name name) override
            {
                std::unique_lock lk{ m_lock };
                VariableResult result;

                auto it = m_map.find(name);
                if (it == m_map.end())
                {
                    result.pData = nullptr;
                    result.Code = VariableResultCode::NotFound;
                    return result;
                }

                result.Code = VariableResultCode::Removed;
                result.pData = it->second;
                m_map.erase(it);
                return result;
            }

            void Destroy() override
            {
#if FE_DEBUG
                if (!m_map.empty())
                {
                    printf("%" PRId64 " global environment variable leaks detected\n", m_map.size());
                    Console::SetTextColor(Console::Color::kRed);
                    puts("Destructors of leaked variables won't be called!");
                    puts("Be sure to free all global variables before global environment shut-down");
                    Console::SetTextColor(Console::Color::kDefault);
                    for (auto& [name, data] : m_map)
                    {
                        printf("%s", name.c_str());
                        Memory::DefaultFree(data);
                    }
                }
#endif

                Memory::DefaultDelete(this);
            }
        };


        static Environment* g_EnvInstance = nullptr;
        static bool g_IsEnvOwner = false;


        DI::ServiceRegistry* CreateServiceRegistry()
        {
            return g_EnvInstance->m_diContainer.GetRegistryRoot()->Create();
        }


        DI::ServiceRegistry* GetRootServiceRegistry()
        {
            return g_EnvInstance->m_diContainer.GetRegistryRoot()->GetRootRegistry();
        }


        SharedState::SharedState()
            : m_threadDataAllocator("NativeThreadData", sizeof(NativeThreadData), 64 * 1024)
        {
        }


        SharedState& SharedState::Get()
        {
            return g_EnvInstance->m_sharedState;
        }
    } // namespace Internal


    std::pmr::memory_resource* GetStaticAllocator(Memory::StaticAllocatorType type)
    {
        return Internal::g_EnvInstance->GetStaticAllocator(type);
    }


    DI::IServiceProvider* GetServiceProvider()
    {
        return &Internal::g_EnvInstance->m_diContainer;
    }


    Name::Name(std::string_view str)
    {
        auto& nameAllocator = Internal::g_EnvInstance->m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        m_handle = nameAllocator.TryFind(hash);
        if (Valid())
        {
            FE_CORE_ASSERT(str == GetRecord()->m_data, "Env::Name collision");
            return;
        }

        const size_t recordHeaderSize = offsetof(Name::Record, m_data);
        FE_CORE_ASSERT(str.size() < Internal::kNamePageByteSize - recordHeaderSize, "Env::Name is too long");

        Record* pRecord = nameAllocator.Allocate(hash, str.size() + 1, m_handle);
        pRecord->m_size = static_cast<uint16_t>(str.size());
        pRecord->m_hash = hash;
        memcpy(pRecord->m_data, str.data(), str.size());
    }


    bool Name::TryGetExisting(std::string_view str, Name& result)
    {
        auto& nameAllocator = Internal::g_EnvInstance->m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        result.m_handle = nameAllocator.TryFind(hash);
        if (result.Valid())
        {
            FE_CORE_ASSERT(str == result.GetRecord()->m_data, "Env::Name collision");
            return true;
        }

        return false;
    }


    const Name::Record* Name::GetRecord() const
    {
        if (!Valid())
            return nullptr;

        return Internal::g_EnvInstance->m_nameDataAllocator.ResolvePointer(m_handle);
    }


    void CreateEnvironment()
    {
        if (Internal::g_EnvInstance)
        {
            return;
        }

        Internal::g_IsEnvOwner = true;
        Internal::g_EnvInstance = Memory::DefaultNew<Internal::Environment>();

        Console::Init();
        Internal::g_EnvInstance->m_diContainer.GetRegistryRoot()->Initialize();
    }


    Internal::IEnvironment& GetEnvironment()
    {
        FE_CORE_ASSERT(Internal::g_EnvInstance, "Environment must be created explicitly");
        return *Internal::g_EnvInstance;
    }


    void AttachEnvironment(Internal::IEnvironment& instance)
    {
        DetachEnvironment();

        Internal::g_EnvInstance = &static_cast<Internal::Environment&>(instance);
        std::pmr::set_default_resource(&Internal::g_EnvInstance->m_defaultMemoryResource);
    }


    void DetachEnvironment()
    {
        if (!Internal::g_EnvInstance)
            return;

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
