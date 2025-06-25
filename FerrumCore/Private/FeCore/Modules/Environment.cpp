#include <FeCore/Base/AssertPrivate.h>
#include <FeCore/Base/Platform.h>
#include <FeCore/Base/StackTracePrivate.h>
#include <FeCore/Compression/CompressionPrivate.h>
#include <FeCore/Console/ConsolePrivate.h>
#include <FeCore/DI/Builder.h>
#include <FeCore/DI/Container.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Memory/MemoryPrivate.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Platform/Windows/Common.h>
#include <FeCore/Threading/SharedSpinLock.h>
#include <FeCore/Threading/Thread.h>
#include <FeCore/Threading/ThreadingPrivate.h>
#include <festd/unordered_map.h>

namespace FE::Env
{
    namespace
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

            Threading::SharedSpinLock m_lock;
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
                std::shared_lock lk{ m_lock };
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
                    FE_CoreAssert(m_currentPageIndex < kMaxPageCount);
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


        struct DefaultMemoryResource final : public std::pmr::memory_resource
        {
            DefaultMemoryResource()
            {
                m_previousResource = std::pmr::get_default_resource();
                std::pmr::set_default_resource(this);
            }

            ~DefaultMemoryResource()
            {
                std::pmr::set_default_resource(m_previousResource);
            }

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

        private:
            std::pmr::memory_resource* m_previousResource = nullptr;
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

            [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        struct Environment final
        {
            struct HighPriorityInitializer final
            {
                HighPriorityInitializer(std::pmr::memory_resource* allocator)
                {
                    tracy::StartupProfiler();

                    Console::Internal::Init(allocator);
                    Trace::Internal::InitStackTrace(allocator);
                    Memory::Internal::Init(allocator);
                    Trace::Internal::Init(allocator);
                    Threading::Internal::Init(allocator);
                    Compression::Internal::Init(allocator);
                }

                ~HighPriorityInitializer()
                {
                    Compression::Internal::Shutdown();
                    Threading::Internal::Shutdown();
                    Trace::Internal::Shutdown();
                    Memory::Internal::Shutdown();
                    Trace::Internal::ShutdownStackTrace();
                    Console::Internal::Shutdown();

                    tracy::ShutdownProfiler();
                }
            };

            VirtualMemoryResource m_virtualMemoryResource;
            Memory::SpinLockedLinearAllocator m_linearMemoryResource;
            DefaultMemoryResource m_defaultMemoryResource;

            HighPriorityInitializer m_highPriorityInitializer;

            NameDataAllocator m_nameDataAllocator;
            DI::Container m_diContainer;

            ApplicationInfo m_appInfo;

            Environment()
                : m_linearMemoryResource(UINT64_C(2) * 1024 * 1024, &m_virtualMemoryResource)
                , m_highPriorityInitializer(&m_linearMemoryResource)
            {
                const Platform::CpuInfo cpuInfo = Platform::GetCpuInfo();
                if (!cpuInfo.MeetsMinimalRequirements())
                {
                    const festd::fixed_string message = Fmt::FixedFormat(
                        "Your CPU {} does not meet minimal requirements. AVX support is required to run application",
                        festd::string_view(cpuInfo.m_cpuName));
                    Platform::FatalInitError(message.c_str());
                }

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
        };


        Module* GModuleList = nullptr;


#pragma warning(disable : 4075)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
        Environment GEnvironment;
    } // namespace


    DI::ServiceRegistry* CreateServiceRegistry()
    {
        return GEnvironment.m_diContainer.GetRegistryRoot()->CreateRegistry();
    }


    DI::ServiceRegistry* GetRootServiceRegistry()
    {
        return GEnvironment.m_diContainer.GetRegistryRoot()->GetRootRegistry();
    }


    std::pmr::memory_resource* GetStaticAllocator(const Memory::StaticAllocatorType type)
    {
        return GEnvironment.GetStaticAllocator(type);
    }


    DI::IServiceProvider* GetServiceProvider()
    {
        return &GEnvironment.m_diContainer;
    }


    Name::Name(const std::string_view str)
    {
        auto& nameAllocator = GEnvironment.m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        m_handle = nameAllocator.TryFind(hash);
        if (IsValid())
        {
            FE_CoreAssert(str == GetRecord()->m_data, "Env::Name collision");
            return;
        }

        const size_t recordHeaderSize = offsetof(Name::Record, m_data);
        FE_CoreAssert(str.size() < kNamePageByteSize - recordHeaderSize, "Env::Name is too long");

        Record* record = nameAllocator.Allocate(hash, str.size() + 1, m_handle);
        record->m_size = static_cast<uint16_t>(str.size());
        record->m_hash = hash;
        memcpy(record->m_data, str.data(), str.size());
    }


    bool Name::TryGetExisting(const std::string_view str, Name& result)
    {
        auto& nameAllocator = GEnvironment.m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        result.m_handle = nameAllocator.TryFind(hash);
        if (result.IsValid())
        {
            FE_CoreAssert(str == result.GetRecord()->m_data, "Env::Name collision");
            return true;
        }

        return false;
    }


    const Name::Record* Name::GetRecord() const
    {
        if (!IsValid())
            return nullptr;

        return GEnvironment.m_nameDataAllocator.ResolvePointer(m_handle);
    }


    void Module::Register(Module* module)
    {
        FE_CoreAssert(!module->m_next, "Module already registered");
        module->m_next = GModuleList;
        GModuleList = module;
    }


    Module* Module::GetModuleList()
    {
        return GModuleList;
    }


    void Module::ShutdownModules()
    {
        Module* module = GetModuleList();
        while (module)
        {
            Module* next = module->m_next;
            module->Shutdown();
            module = next;
        }

        GModuleList = nullptr;
    }


    void Init(const ApplicationInfo& info)
    {
        FE_CoreAssert(!GEnvironment.m_appInfo.m_name, "Application info already set");
        GEnvironment.m_appInfo = info;

        DI::ServiceRegistryBuilder builder{ GetRootServiceRegistry() };
        DI::RegisterCoreServices(builder);
        builder.Build();
    }


    const ApplicationInfo& GetApplicationInfo()
    {
        return GEnvironment.m_appInfo;
    }
} // namespace FE::Env
