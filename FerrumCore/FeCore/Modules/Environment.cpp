#include <FeCore/Console/Console.h>
#include <FeCore/Containers/HashTables.h>
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

            inline bool do_is_equal(const memory_resource&) const noexcept override
            {
                return false;
            }
        };

        class Environment : public IEnvironment
        {
            using Map = SortedStringVector<void*, std::vector<char>, std::string_view>;

            Map m_Map;
            Mutex m_Lock;

            inline VariableResult FindVariableNoLock(std::string_view name)
            {
                VariableResult result;
                auto it = m_Map.FindCIter(name);
                if (it != m_Map.end())
                {
                    auto& vec = std::get<0>(*it);
                    auto view = std::string_view(vec.data(), vec.size());
                    if (view == name)
                    {
                        result.Code = VariableResultCode::Found;
                        result.pData = std::get<1>(*it);
                        return result;
                    }
                }

                result.Code = VariableResultCode::NotFound;
                result.pData = nullptr;
                return result;
            }

        public:
            DefaultMemoryResource DefaultMemoryResource;
            NameDataAllocator NameDataAllocator;

            inline Environment()
            {
                std::pmr::set_default_resource(&DefaultMemoryResource);

                Console::Init();
                Console::SetColor(Console::Color::Green);
                // std::stringstream ss;
                // ss << "====================[ Ferrum v" << FerrumVersion.Major << "." << FerrumVersion.Minor;
                // ss << " Global environment created ]====================\n";
                // puts(ss.str().c_str());
                Console::ResetColor();
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
                if (find.pData)
                    return find;

                void* storage = Memory::DefaultAllocate(size, alignment);
                auto& [str, ptr] = m_Map.Push(std::move(name), storage);
                nameView = std::string_view(str.data(), str.size());

                VariableResult result;
                result.Code = VariableResultCode::Created;
                result.pData = storage;
                return result;
            }

            inline VariableResult RemoveVariable(std::string_view name) override
            {
                std::unique_lock lk(m_Lock);
                VariableResult result;

                auto it = m_Map.FindIter(name);
                if (it == m_Map.end())
                {
                    result.pData = nullptr;
                    result.Code = VariableResultCode::NotFound;
                    return result;
                }

                result.Code = VariableResultCode::Removed;
                result.pData = std::get<1>(*it);

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


        static Environment* g_EnvInstance = nullptr;
        static bool g_IsEnvOwner = false;
    } // namespace Internal


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

        FE_CORE_ASSERT(str.size() < 0xffff, "Env::Name is too long");

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
