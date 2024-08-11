#include <FeCore/Console/Console.h>
#include <FeCore/Containers/HashTables.h>
#include <FeCore/DI/Container.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Modules/EnvironmentPrivate.h>
#include <FeCore/Parallel/Thread.h>
#include <cstdio>
#include <sstream>

namespace FE::Env
{
    namespace Internal
    {
        inline constexpr uint32_t NamePageShift = 16;
        inline constexpr uint32_t NamePageByteSize = 1 << NamePageShift;
        inline constexpr uint32_t NameBlockShift = 3;
        static_assert(1 << NameBlockShift == alignof(Name::Record));


        class NameDataAllocator final
        {
            struct NameHandle final
            {
                uint32_t BlockIndex : NamePageShift - NameBlockShift;
                uint32_t PageIndex : 32 - NamePageShift + NameBlockShift;
            };

            inline static constexpr uint32_t PageListByteSize = 64 * 1024;
            inline static constexpr uint32_t MaxPageCount = PageListByteSize / sizeof(void*);

            SpinLock m_Lock;
            void** m_ppPages;
            uint32_t m_CurrentPageIndex = InvalidIndex;
            uint32_t m_Offset = NamePageByteSize;
            festd::unordered_dense_map<uint64_t, uint32_t> m_Map;

        public:
            inline NameDataAllocator()
            {
                m_ppPages = static_cast<void**>(Memory::AllocateVirtual(PageListByteSize));
            }

            inline ~NameDataAllocator()
            {
                while (m_CurrentPageIndex != InvalidIndex)
                {
                    Memory::FreeVirtual(m_ppPages[m_CurrentPageIndex], NamePageByteSize);
                    --m_CurrentPageIndex;
                }

                Memory::FreeVirtual(m_ppPages, PageListByteSize);
            }

            inline uint32_t TryFind(uint64_t hash)
            {
                std::lock_guard lk{ m_Lock };
                const auto it = m_Map.find(hash);
                if (it != m_Map.end())
                    return it->second;
                return InvalidIndex;
            }

            inline Name::Record* Allocate(uint64_t hash, size_t stringByteSize, uint32_t& handle)
            {
                std::lock_guard lk{ m_Lock };
                const size_t recordHeaderSize = offsetof(Name::Record, Data);
                const size_t recordSize = AlignUp<1 << NameBlockShift>(recordHeaderSize + stringByteSize);
                if (recordSize + m_Offset > NamePageByteSize)
                {
                    m_ppPages[++m_CurrentPageIndex] = Memory::AllocateVirtual(NamePageByteSize);
                    m_Offset = 0;
                }

                NameHandle result;
                result.PageIndex = m_CurrentPageIndex;
                result.BlockIndex = m_Offset >> NameBlockShift;
                handle = bit_cast<uint32_t>(result);
                m_Map.insert(std::make_pair(hash, handle));

                void* ptr = static_cast<uint8_t*>(m_ppPages[m_CurrentPageIndex]) + m_Offset;
                m_Offset += static_cast<uint32_t>(recordSize);
                FE_CORE_ASSERT((m_Offset >> NameBlockShift) << NameBlockShift == m_Offset, "");
                return static_cast<Name::Record*>(ptr);
            }

            inline Name::Record* ResolvePointer(uint32_t handleValue)
            {
                const NameHandle handle = bit_cast<NameHandle>(handleValue);
                const uintptr_t pageAddress = reinterpret_cast<uintptr_t>(m_ppPages[handle.PageIndex]);
                const uintptr_t recordAddress = pageAddress + (static_cast<size_t>(handle.BlockIndex) << NameBlockShift);
                return reinterpret_cast<Name::Record*>(recordAddress);
            }
        };


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

            inline bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        class VirtualMemoryResource final : public std::pmr::memory_resource
        {
        public:
            inline void* do_allocate(size_t size, size_t alignment) override
            {
                FE_CORE_ASSERT(Memory::GetPlatformSpec().Granularity >= alignment, "Unsupported alignment");
                return Memory::AllocateVirtual(size);
            }

            inline void do_deallocate(void* p, size_t size, size_t) override
            {
                return Memory::FreeVirtual(p, size);
            }

            inline bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        class Environment : public IEnvironment
        {
            festd::unordered_dense_map<Name, void*> m_Map;
            Mutex m_Lock;

            inline VariableResult FindVariableNoLock(Name name)
            {
                VariableResult result;
                auto it = m_Map.find(name);
                if (it != m_Map.end())
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
            DefaultMemoryResource DefaultMemoryResource;
            VirtualMemoryResource VirtualMemoryResource;
            Memory::LockedMemoryResource<Memory::LinearAllocator, SpinLock> LinearMemoryResource;
            Memory::PoolAllocator ThreadDataPool{ "NativeThreadData", sizeof(NativeThreadData), 64 * 1024 };

            NameDataAllocator NameDataAllocator;
            DI::Container DIContainer;

            inline Environment()
                : LinearMemoryResource(256 * 1024, &VirtualMemoryResource)
            {
                std::pmr::set_default_resource(&DefaultMemoryResource);

                Console::Init();
                Console::ResetColor();
            }

            inline std::pmr::memory_resource* GetStaticAllocator(Memory::StaticAllocatorType type)
            {
                switch (type)
                {
                case Memory::StaticAllocatorType::Default:
                    return &DefaultMemoryResource;
                case Memory::StaticAllocatorType::Virtual:
                    return &VirtualMemoryResource;
                case Memory::StaticAllocatorType::Linear:
                    return &LinearMemoryResource;
                default:
                    FE_DEBUGBREAK;
                    return nullptr;
                }
            }

            inline VariableResult FindVariable(Name name) override
            {
                std::unique_lock lk{ m_Lock };
                return FindVariableNoLock(name);
            }

            inline VariableResult CreateVariable(Name name, size_t size, size_t alignment) override
            {
                std::unique_lock lk{ m_Lock };
                VariableResult result = FindVariableNoLock(name);
                if (result.Code == VariableResultCode::Found)
                    return result;

                result.Code = VariableResultCode::Created;
                result.pData = Memory::DefaultAllocate(size, alignment);
                m_Map[name] = result.pData;
                return result;
            }

            inline VariableResult RemoveVariable(Name name) override
            {
                std::unique_lock lk{ m_Lock };
                VariableResult result;

                auto it = m_Map.find(name);
                if (it == m_Map.end())
                {
                    result.pData = nullptr;
                    result.Code = VariableResultCode::NotFound;
                    return result;
                }

                result.Code = VariableResultCode::Removed;
                result.pData = it->second;
                m_Map.erase(it);
                return result;
            }

            inline void Destroy() override
            {
#if FE_DEBUG
                if (!m_Map.empty())
                {
                    printf("%" PRId64 " global environment variable leaks detected\n", m_Map.size());
                    Console::SetColor(Console::Color::Red);
                    puts("Destructors of leaked variables won't be called!");
                    puts("Be sure to free all global variables before global environment shut-down");
                    Console::ResetColor();
                    for (auto& [name, data] : m_Map)
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
            return Internal::g_EnvInstance->DIContainer.GetRegistryRoot()->Create();
        }


        DI::ServiceRegistry* GetRootServiceRegistry()
        {
            return Internal::g_EnvInstance->DIContainer.GetRegistryRoot()->GetRootRegistry();
        }


        Memory::PoolAllocator* GetThreadDataPool()
        {
            return &Internal::g_EnvInstance->ThreadDataPool;
        }
    } // namespace Internal

    std::pmr::memory_resource* GetStaticAllocator(Memory::StaticAllocatorType type)
    {
        return Internal::g_EnvInstance->GetStaticAllocator(type);
    }


    DI::IServiceProvider* FE::Env::GetServiceProvider()
    {
        return &Internal::g_EnvInstance->DIContainer;
    }


    Name::Name(std::string_view str)
    {
        auto& nameAllocator = Internal::g_EnvInstance->NameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        m_Handle = nameAllocator.TryFind(hash);
        if (Valid())
        {
            if (IsDebugBuild)
            {
                FE_CORE_ASSERT(str == GetRecord()->Data, "Env::Name collision");
            }

            return;
        }

        const size_t recordHeaderSize = offsetof(Name::Record, Data);
        FE_CORE_ASSERT(str.size() < Internal::NamePageByteSize - recordHeaderSize, "Env::Name is too long");

        Name::Record* pRecord = nameAllocator.Allocate(hash, str.size() + 1, m_Handle);
        pRecord->Size = static_cast<uint16_t>(str.size());
        pRecord->Hash = hash;
        memcpy(pRecord->Data, str.data(), str.size());
    }


    const Name::Record* Name::GetRecord() const
    {
        if (!Valid())
            return nullptr;

        return Internal::g_EnvInstance->NameDataAllocator.ResolvePointer(m_Handle);
    }


    void CreateEnvironment()
    {
        if (Internal::g_EnvInstance)
        {
            return;
        }

        Internal::g_IsEnvOwner = true;
        Internal::g_EnvInstance = Memory::DefaultNew<Internal::Environment>();
        Internal::g_EnvInstance->DIContainer.GetRegistryRoot()->Initialize();
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
        std::pmr::set_default_resource(&Internal::g_EnvInstance->DefaultMemoryResource);
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
