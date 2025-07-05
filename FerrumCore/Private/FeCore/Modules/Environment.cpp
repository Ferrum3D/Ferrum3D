#include <FeCore/Base/Platform.h>
#include <FeCore/Compression/CompressionInternal.h>
#include <FeCore/Console/Console.h>
#include <FeCore/DI/Container.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Modules/EnvironmentPrivate.h>
#include <FeCore/Modules/IModule.h>
#include <FeCore/Platform/Windows/Common.h>
#include <FeCore/Threading/Platform/ThreadingInternal.h>
#include <FeCore/Threading/Thread.h>
#include <cstdio>
#include <festd/unordered_map.h>
#include <sstream>

namespace FE::Env
{
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
                uint32_t m_blockIndex : kNamePageShift - kNameBlockShift;
                uint32_t m_pageIndex : 32 - kNamePageShift + kNameBlockShift;
            };

            static constexpr uint32_t kPageListByteSize = 64 * 1024;
            static constexpr uint32_t kMaxPageCount = kPageListByteSize / sizeof(void*);

            Threading::SpinLock m_lock;
            void** m_pages;
            uint32_t m_currentPageIndex = kInvalidIndex;
            uint32_t m_offset = kNamePageByteSize;
            festd::unordered_dense_map<uint64_t, uint32_t> m_map;

        public:
            NameDataAllocator()
            {
                m_pages = static_cast<void**>(Memory::AllocateVirtual(kPageListByteSize));
            }

            ~NameDataAllocator()
            {
                while (m_currentPageIndex != kInvalidIndex)
                {
                    Memory::FreeVirtual(m_pages[m_currentPageIndex], kNamePageByteSize);
                    --m_currentPageIndex;
                }

                Memory::FreeVirtual(static_cast<void*>(m_pages), kPageListByteSize);
            }

            uint32_t TryFind(const uint64_t hash)
            {
                std::lock_guard lk{ m_lock };
                const auto it = m_map.find(hash);
                if (it != m_map.end())
                    return it->second;
                return kInvalidIndex;
            }

            Name::Record* Allocate(uint64_t hash, const size_t stringByteSize, uint32_t& handle)
            {
                std::lock_guard lk{ m_lock };
                const size_t recordHeaderSize = offsetof(Name::Record, m_data);
                const size_t recordSize = AlignUp<1 << kNameBlockShift>(recordHeaderSize + stringByteSize);
                if (recordSize + m_offset > kNamePageByteSize)
                {
                    m_pages[++m_currentPageIndex] = Memory::AllocateVirtual(kNamePageByteSize);
                    m_offset = 0;
                }

                NameHandle result;
                result.m_pageIndex = m_currentPageIndex;
                result.m_blockIndex = m_offset >> kNameBlockShift;
                handle = festd::bit_cast<uint32_t>(result);
                m_map.insert(std::make_pair(hash, handle));

                void* ptr = static_cast<uint8_t*>(m_pages[m_currentPageIndex]) + m_offset;
                m_offset += static_cast<uint32_t>(recordSize);
                FE_CoreAssert((m_offset >> kNameBlockShift) << kNameBlockShift == m_offset);
                return static_cast<Name::Record*>(ptr);
            }

            Name::Record* ResolvePointer(const uint32_t handleValue) const
            {
                const NameHandle handle = festd::bit_cast<NameHandle>(handleValue);
                const uintptr_t pageAddress = reinterpret_cast<uintptr_t>(m_pages[handle.m_pageIndex]);
                const uintptr_t recordAddress = pageAddress + (static_cast<size_t>(handle.m_blockIndex) << kNameBlockShift);
                return reinterpret_cast<Name::Record*>(recordAddress);
            }
        };


        class DefaultMemoryResource final : public std::pmr::memory_resource
        {
        public:
            void* do_allocate(const size_t size, const size_t alignment) override
            {
                return Memory::DefaultAllocate(size, alignment);
            }

            void do_deallocate(void* p, size_t, size_t) override
            {
                Memory::DefaultFree(p);
            }

            [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        class VirtualMemoryResource final : public std::pmr::memory_resource
        {
        public:
            void* do_allocate(const size_t size, const size_t alignment) override
            {
                FE_CoreAssert(Memory::GetPlatformSpec().m_granularity >= alignment, "Unsupported alignment");
                return Memory::AllocateVirtual(size);
            }

            void do_deallocate(void* p, const size_t size, size_t) override
            {
                return Memory::FreeVirtual(p, size);
            }

            bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        struct Environment final
        {
            SharedState m_sharedState;

            ApplicationInfo m_appInfo;

            DefaultMemoryResource m_defaultMemoryResource;
            VirtualMemoryResource m_virtualMemoryResource;
            Memory::LockedMemoryResource<Memory::LinearAllocator, Threading::SpinLock> m_linearMemoryResource;

            NameDataAllocator m_nameDataAllocator;
            DI::Container m_diContainer;
            ModuleRegistry m_moduleRegistry;

            Threading::Mutex m_lock;

            Environment(const ApplicationInfo& appInfo)
                : m_appInfo(appInfo)
                , m_linearMemoryResource(UINT64_C(2) * 1024 * 1024, &m_virtualMemoryResource)
            {
                std::pmr::set_default_resource(&m_defaultMemoryResource);
            }

            void Init()
            {
                Console::Init();
                Threading::Internal::Init();
                Compression::Internal::Init();

                m_diContainer.GetRegistryRoot()->Initialize();
            }

            std::pmr::memory_resource* GetStaticAllocator(const Memory::StaticAllocatorType type)
            {
                switch (type)
                {
                case Memory::StaticAllocatorType::kDefault:
                    return &m_defaultMemoryResource;
                case Memory::StaticAllocatorType::kVirtual:
                    return &m_virtualMemoryResource;
                case Memory::StaticAllocatorType::kLinear:
                    return &m_linearMemoryResource;
                default:
                    FE_DebugBreak();
                    return nullptr;
                }
            }

            void Destroy()
            {
                Compression::Internal::Shutdown();
                this->~Environment();
            }
        };


        static Environment* GEnvInstance = nullptr;
        static bool GIsEnvOwner = false;
        static std::byte GEnvInstanceStorage[sizeof(Environment)];


        DebugHeapHolder::DebugHeapHolder()
        {
            constexpr size_t kGigabyte = 0x40000000;
            if (Build::IsDevelopment())
                m_heap = DebugHeapInit(8 * kGigabyte);
        }


        DebugHeapHolder::~DebugHeapHolder()
        {
            if (m_heap)
                DebugHeapDestroy(m_heap);
        }


        SharedState::SharedState()
            : m_mainThreadId(Threading::GetCurrentThreadID())
        {
        }


        SharedState& SharedState::Get()
        {
            return GEnvInstance->m_sharedState;
        }
    } // namespace Internal


    DI::ServiceRegistry* CreateServiceRegistry()
    {
        return Internal::GEnvInstance->m_diContainer.GetRegistryRoot()->Create();
    }


    DI::ServiceRegistry* GetRootServiceRegistry()
    {
        return Internal::GEnvInstance->m_diContainer.GetRegistryRoot()->GetRootRegistry();
    }


    std::pmr::memory_resource* GetStaticAllocator(const Memory::StaticAllocatorType type)
    {
        return Internal::GEnvInstance->GetStaticAllocator(type);
    }


    DI::IServiceProvider* GetServiceProvider()
    {
        return &Internal::GEnvInstance->m_diContainer;
    }


    ModuleRegistry* GetModuleRegistry()
    {
        return &Internal::GEnvInstance->m_moduleRegistry;
    }


    Name::Name(const std::string_view str)
    {
        auto& nameAllocator = Internal::GEnvInstance->m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        m_handle = nameAllocator.TryFind(hash);
        if (Valid())
        {
            FE_CoreAssert(str == GetRecord()->m_data, "Env::Name collision");
            return;
        }

        const size_t recordHeaderSize = offsetof(Name::Record, m_data);
        FE_CoreAssert(str.size() < Internal::kNamePageByteSize - recordHeaderSize, "Env::Name is too long");

        Record* record = nameAllocator.Allocate(hash, str.size() + 1, m_handle);
        record->m_size = static_cast<uint16_t>(str.size());
        record->m_hash = hash;
        memcpy(record->m_data, str.data(), str.size());
    }


    bool Name::TryGetExisting(const std::string_view str, Name& result)
    {
        auto& nameAllocator = Internal::GEnvInstance->m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        result.m_handle = nameAllocator.TryFind(hash);
        if (result.Valid())
        {
            FE_CoreAssert(str == result.GetRecord()->m_data, "Env::Name collision");
            return true;
        }

        return false;
    }


    const Name::Record* Name::GetRecord() const
    {
        if (!Valid())
            return nullptr;

        return Internal::GEnvInstance->m_nameDataAllocator.ResolvePointer(m_handle);
    }


    void CreateEnvironment(const ApplicationInfo& info)
    {
        tracy::StartupProfiler();

        {
            FE_PROFILER_ZONE();

            if (Internal::GEnvInstance)
            {
                return;
            }

            const Platform::CpuInfo cpuInfo = Platform::GetCpuInfo();
            if (!cpuInfo.MeetsMinimalRequirements())
            {
                const festd::fixed_string message =
                    Fmt::FixedFormat("Your CPU {} does not meet minimal requirements. AVX support is required to run application",
                                     festd::string_view(cpuInfo.m_cpuName));
                Platform::FatalInitError(message.c_str());
            }

            Internal::GIsEnvOwner = true;
            Internal::GEnvInstance = reinterpret_cast<Internal::Environment*>(Internal::GEnvInstanceStorage);
            new (Internal::GEnvInstanceStorage) Internal::Environment(info);
            Internal::GEnvInstance->Init();
        }
    }


    EnvironmentHandle GetEnvironment()
    {
        FE_CoreAssert(Internal::GEnvInstance, "Environment must be created explicitly");
        return EnvironmentHandle{ reinterpret_cast<uintptr_t>(Internal::GEnvInstance) };
    }


    void AttachEnvironment(const EnvironmentHandle instance)
    {
        DetachEnvironment();

        Internal::GEnvInstance = reinterpret_cast<Internal::Environment*>(instance.m_value);
        std::pmr::set_default_resource(&Internal::GEnvInstance->m_defaultMemoryResource);
    }


    void DetachEnvironment()
    {
        if (!Internal::GEnvInstance)
            return;

        if (Internal::GIsEnvOwner)
        {
            Internal::GEnvInstance->Destroy();

            tracy::ShutdownProfiler();
        }

        Internal::GEnvInstance = nullptr;
    }


    [[maybe_unused]] struct EnvironmentRelease
    {
        ~EnvironmentRelease()
        {
            DetachEnvironment();
        }
    } g_EnvRelease;


    bool IsEnvironmentAttached()
    {
        return Internal::GEnvInstance != nullptr;
    }


    const ApplicationInfo& GetApplicationInfo()
    {
        return Internal::GEnvInstance->m_appInfo;
    }
} // namespace FE::Env
