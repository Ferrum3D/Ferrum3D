#include <Core/Base/AssertPrivate.h>
#include <Core/Base/Platform.h>
#include <Core/Base/StackTracePrivate.h>
#include <Core/CLI/CommandLine.h>
#include <Core/Compression/CompressionPrivate.h>
#include <Core/Env/Environment.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AsyncImpl.h>
#include <Core/IO/BaseIOPrivate.h>
#include <Core/Jobs/JobSystem.h>
#include <Core/Logging/LoggerPrivate.h>
#include <Core/Memory/LinearAllocator.h>
#include <Core/Memory/Memory.h>
#include <Core/Memory/MemoryPrivate.h>
#include <Core/Platform/Windows/Common.h>
#include <Core/RTTI/ReflectionPrivate.h>
#include <Core/Threading/SharedSpinLock.h>
#include <Core/Threading/Thread.h>
#include <Core/Threading/ThreadingPrivate.h>
#include <festd/unordered_map.h>

namespace FE
{
    namespace
    {
        inline constexpr uint32_t kNamePageShift = 16;
        inline constexpr uint32_t kNamePageByteSize = 1 << kNamePageShift;
        inline constexpr uint32_t kNameBlockShift = 3;
        static_assert(1 << kNameBlockShift == alignof(Env::Name::Record));


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

            Env::Name::Record* Allocate(uint64_t hash, const size_t stringByteSize, uint32_t& handle)
            {
                std::lock_guard lk{ m_lock };
                const size_t recordHeaderSize = offsetof(Env::Name::Record, m_data);
                const size_t recordSize = AlignUp<1 << kNameBlockShift>(recordHeaderSize + stringByteSize);
                if (recordSize + m_offset > kNamePageByteSize)
                {
                    m_pages[++m_currentPageIndex] = Memory::AllocateVirtual(kNamePageByteSize);
                    FE_Assert(m_currentPageIndex < kMaxPageCount);
                    m_offset = 0;
                }

                NameHandle result;
                result.m_pageIndex = m_currentPageIndex;
                result.m_blockIndex = m_offset >> kNameBlockShift;
                handle = std::bit_cast<uint32_t>(result);
                m_map.insert(std::make_pair(hash, handle));

                void* ptr = static_cast<uint8_t*>(m_pages[m_currentPageIndex]) + m_offset;
                m_offset += static_cast<uint32_t>(recordSize);
                FE_Assert((m_offset >> kNameBlockShift) << kNameBlockShift == m_offset);
                return static_cast<Env::Name::Record*>(ptr);
            }

            Env::Name::Record* ResolvePointer(const uint32_t handleValue) const
            {
                const NameHandle handle = std::bit_cast<NameHandle>(handleValue);
                const uintptr_t pageAddress = reinterpret_cast<uintptr_t>(m_pages[handle.m_pageIndex]);
                const uintptr_t recordAddress = pageAddress + (static_cast<size_t>(handle.m_blockIndex) << kNameBlockShift);
                return reinterpret_cast<Env::Name::Record*>(recordAddress);
            }
        };


        struct DefaultMemoryResource final : public std::pmr::memory_resource
        {
            DefaultMemoryResource()
            {
                m_previousResource = std::pmr::get_default_resource();
                std::pmr::set_default_resource(this);
            }

            ~DefaultMemoryResource() override
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
                FE_Assert(Memory::GetPlatformSpec().m_granularity >= alignment, "Unsupported alignment");
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


#define FE_CORE_SYSTEM(name)                                                                                                     \
    struct FE_UNIQUE_IDENT(CoreSystemInitializerScope)                                                                           \
    {                                                                                                                            \
        FE_UNIQUE_IDENT(CoreSystemInitializerScope)(std::pmr::memory_resource * allocator)                                       \
        {                                                                                                                        \
            name::Internal::Init(allocator);                                                                                     \
        }                                                                                                                        \
        ~FE_UNIQUE_IDENT(CoreSystemInitializerScope)()                                                                           \
        {                                                                                                                        \
            name::Internal::Shutdown();                                                                                          \
        }                                                                                                                        \
    } FE_UNIQUE_IDENT(m_coreSystemInitializerScope)                                                                              \
    {                                                                                                                            \
        &m_linearMemoryResource                                                                                                  \
    }


        struct Environment final
        {
            VirtualMemoryResource m_virtualMemoryResource;
            Memory::SpinLockedLinearAllocator m_linearMemoryResource{ UINT64_C(4) * 1024 * 1024, &m_virtualMemoryResource };
            DefaultMemoryResource m_defaultMemoryResource;

#if FE_DEVELOPMENT
            [[no_unique_address]] struct TracyInitializer final
            {
                TracyInitializer()
                {
                    tracy::StartupProfiler();
                }

                ~TracyInitializer()
                {
                    tracy::ShutdownProfiler();
                }
            } m_tracyInitializer;
#endif

            FE_CORE_SYSTEM(IO);
            FE_CORE_SYSTEM(Trace::StackTrace);
            FE_CORE_SYSTEM(Memory);
            FE_CORE_SYSTEM(Trace);

            NameDataAllocator m_nameDataAllocator;

            FE_CORE_SYSTEM(Logger);
            FE_CORE_SYSTEM(Threading);
            FE_CORE_SYSTEM(Jobs);
            FE_CORE_SYSTEM(Compression);
            FE_CORE_SYSTEM(Rtti::TypeRegistry);
            FE_CORE_SYSTEM(IO::Async);

            Env::ApplicationInfo m_appInfo;
            festd::span<const festd::string_view> m_commandLineArgs;

            Environment()
            {
                const Platform::CpuInfo cpuInfo = Platform::GetCpuInfo();
                if (!cpuInfo.MeetsMinimalRequirements())
                {
                    const festd::fixed_string message = Fmt::FixedFormat(
                        "Your CPU {} does not meet minimal requirements. AVX support is required to run application",
                        festd::string_view(cpuInfo.m_cpuName));
                    Platform::FatalInitError(message.c_str());
                }
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


        Env::Module* GModuleList = nullptr;


#pragma warning(disable : 4075)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
        Environment GEnvironment;
    } // namespace


    std::pmr::memory_resource* Env::GetStaticAllocator(const Memory::StaticAllocatorType type)
    {
        return GEnvironment.GetStaticAllocator(type);
    }


    Env::Name::Name(const std::string_view str)
    {
        auto& nameAllocator = GEnvironment.m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        m_handle = nameAllocator.TryFind(hash);
        if (IsValid())
        {
            FE_Assert(str == GetRecord()->m_data, "Env::Name collision");
            return;
        }

        const size_t recordHeaderSize = offsetof(Name::Record, m_data);
        FE_Assert(str.size() < kNamePageByteSize - recordHeaderSize, "Env::Name is too long");

        Record* record = nameAllocator.Allocate(hash, str.size() + 1, m_handle);
        record->m_size = static_cast<uint16_t>(str.size());
        record->m_hash = hash;
        memcpy(record->m_data, str.data(), str.size());
    }


    bool Env::Name::TryGetExisting(const std::string_view str, Name& result)
    {
        auto& nameAllocator = GEnvironment.m_nameDataAllocator;
        const uint64_t hash = DefaultHash(str);
        result.m_handle = nameAllocator.TryFind(hash);
        if (result.IsValid())
        {
            FE_Assert(str == result.GetRecord()->m_data, "Env::Name collision");
            return true;
        }

        return false;
    }


    const Env::Name::Record* Env::Name::GetRecord() const
    {
        if (!IsValid())
            return nullptr;

        return GEnvironment.m_nameDataAllocator.ResolvePointer(m_handle);
    }


    void Env::Module::Register(Module* module)
    {
        FE_Assert(!module->m_next, "Module already registered");
        module->m_next = GModuleList;
        GModuleList = module;
    }


    Env::Module* Env::Module::GetModuleList()
    {
        return GModuleList;
    }


    void Env::Module::ShutdownModules()
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


    void Env::Init(const ApplicationInfo& info, const int32_t argc, const char** argv)
    {
        FE_Assert(GEnvironment.m_appInfo.m_name == nullptr, "Application info already set");
        FE_Assert(info.m_name != nullptr);
        GEnvironment.m_appInfo = info;

        const uint32_t argCount = argc - 1;
        auto* args = Memory::AllocateArray<festd::string_view>(&GEnvironment.m_linearMemoryResource, argCount);
        for (uint32_t argIndex = 0; argIndex < argCount; ++argIndex)
            args[argIndex] = argv[argIndex + 1];

        GEnvironment.m_commandLineArgs = festd::span(args, argCount);

        IO::ArtifactStore::Init();
    }


    void Env::Shutdown()
    {
        IO::ArtifactStore::Shutdown();
    }


    const Env::ApplicationInfo& Env::GetApplicationInfo()
    {
        return GEnvironment.m_appInfo;
    }


    festd::span<const festd::string_view> Cli::GetArgs()
    {
        return GEnvironment.m_commandLineArgs;
    }
} // namespace FE
