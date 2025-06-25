#pragma once
#include <FeCore/DI/BaseDI.h>

namespace FE::Memory
{
    //! @brief Type of global static allocator.
    enum class StaticAllocatorType : uint32_t
    {
        kDefault, //!< Default global heap allocator, can also be obtained with `std::pmr::get_default_resource()`.
        kVirtual, //!< Allocates virtual memory directly from the OS.
        kLinear,  //!< Global linear allocator, the allocated memory will be freed only upon process termination.
    };
} // namespace FE::Memory


namespace FE::Env
{
    //! @brief A shared string with fast equality comparison that is never deallocated.
    struct Name final
    {
        struct Record final
        {
            uint64_t m_hash; // 64-bit hash of the string.
            uint16_t m_size; // Size of the string in bytes.
            char m_data[1];  // Actual string is longer, but starts here.
        };

        Name() = default;

        explicit Name(std::string_view str);

        Name(const char* data, const uint32_t length)
            : Name(std::string_view{ data, length ? length : __builtin_strlen(data) })
        {
        }

        template<uint32_t TSize>
        Name(const char (&data)[TSize])
            : Name(std::string_view{ data, __builtin_strlen(data) })
        {
        }

        [[nodiscard]] static bool TryGetExisting(std::string_view str, Name& result);

        [[nodiscard]] static Name CreateFromHandle(const uint32_t handle)
        {
            return festd::bit_cast<Name>(handle);
        }

        [[nodiscard]] const Record* GetRecord() const;

        [[nodiscard]] uint32_t GetHandle() const
        {
            return m_handle;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            constexpr uint64_t kEmptyStringHash = CompileTimeHash("", 0);
            return IsValid() ? GetRecord()->m_hash : kEmptyStringHash;
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_handle != kInvalidIndex;
        }

        [[nodiscard]] uint32_t size() const
        {
            return IsValid() ? GetRecord()->m_size : 0;
        }

        [[nodiscard]] const char* c_str() const
        {
            return IsValid() ? GetRecord()->m_data : nullptr;
        }

        [[nodiscard]] int32_t compare(const char* s) const
        {
            const Record* record = GetRecord();
            return strncmp(record->m_data, s, record->m_size);
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        explicit operator festd::ascii_view() const
        {
            const Record* record = GetRecord();
            return record ? festd::ascii_view{ record->m_data, record->m_size } : festd::ascii_view{};
        }

        friend bool operator==(const Name lhs, const Name rhs)
        {
            return lhs.m_handle == rhs.m_handle;
        }
        friend bool operator!=(const Name lhs, const Name rhs)
        {
            return lhs.m_handle != rhs.m_handle;
        }

        friend bool operator==(const Name lhs, const char* rhs)
        {
            return lhs.compare(rhs) == 0;
        }
        friend bool operator!=(const Name lhs, const char* rhs)
        {
            return lhs.compare(rhs) != 0;
        }
        friend bool operator<(const Name lhs, const char* rhs)
        {
            return lhs.compare(rhs) < 0;
        }
        friend bool operator>(const Name lhs, const char* rhs)
        {
            return lhs.compare(rhs) > 0;
        }
        friend bool operator<=(const Name lhs, const char* rhs)
        {
            return lhs.compare(rhs) <= 0;
        }
        friend bool operator>=(const Name lhs, const char* rhs)
        {
            return lhs.compare(rhs) >= 0;
        }

        friend bool operator==(const char* lhs, const Name rhs)
        {
            return rhs.compare(lhs) == 0;
        }
        friend bool operator!=(const char* lhs, const Name rhs)
        {
            return rhs.compare(lhs) != 0;
        }
        friend bool operator<(const char* lhs, const Name rhs)
        {
            return rhs.compare(lhs) > 0;
        }
        friend bool operator>(const char* lhs, const Name rhs)
        {
            return rhs.compare(lhs) < 0;
        }
        friend bool operator<=(const char* lhs, const Name rhs)
        {
            return rhs.compare(lhs) >= 0;
        }
        friend bool operator>=(const char* lhs, const Name rhs)
        {
            return rhs.compare(lhs) <= 0;
        }

        static const Name kEmpty;

    private:
        uint32_t m_handle = kInvalidIndex;
    };

    inline const Name Name::kEmpty;


    struct Module
    {
        virtual ~Module() = default;
        static Module* GetModuleList();
        static void ShutdownModules();

        virtual void RegisterServices([[maybe_unused]] const DI::ServiceRegistryBuilder& builder) {}

        Module* m_next = nullptr;
        DI::ServiceRegistry* m_serviceRegistry = nullptr;

    protected:
        virtual void Shutdown() = 0;
        static void Register(Module* module);
    };

#define FE_DECLARE_MODULE(name)                                                                                                  \
    static name* GInstance;                                                                                                      \
    static void Init();                                                                                                          \
    void Shutdown() override;                                                                                                    \
    struct FE_UNIQUE_IDENT(Dummy_DeclareModule)                                                                                  \
    {                                                                                                                            \
    }

#define FE_IMPLEMENT_MODULE(name)                                                                                                \
    name* name::GInstance = nullptr;                                                                                             \
    void name::Init()                                                                                                            \
    {                                                                                                                            \
        GInstance = FE::Memory::New<name>(FE::Env::GetStaticAllocator(FE::Memory::StaticAllocatorType::kLinear));                \
        GInstance->m_serviceRegistry = FE::Env::CreateServiceRegistry();                                                         \
        GInstance->m_serviceRegistry->AddRef();                                                                                  \
        Register(GInstance);                                                                                                     \
    }                                                                                                                            \
    void name::Shutdown()                                                                                                        \
    {                                                                                                                            \
        if (GInstance)                                                                                                           \
        {                                                                                                                        \
            GInstance->m_serviceRegistry->Release();                                                                             \
            GInstance->~name();                                                                                                  \
        }                                                                                                                        \
        GInstance = nullptr;                                                                                                     \
    }                                                                                                                            \
    struct FE_UNIQUE_IDENT(Dummy_ImplementModule)                                                                                \
    {                                                                                                                            \
    }


    struct ApplicationInfo final
    {
        const char* m_name = nullptr;
    };

    void Init(const ApplicationInfo& info);
    const ApplicationInfo& GetApplicationInfo();

    std::pmr::memory_resource* GetStaticAllocator(Memory::StaticAllocatorType type);
    DI::IServiceProvider* GetServiceProvider();

    DI::ServiceRegistry* CreateServiceRegistry();
    DI::ServiceRegistry* GetRootServiceRegistry();
} // namespace FE::Env


template<>
struct eastl::hash<FE::Env::Name>
{
    size_t operator()(const FE::Env::Name name) const
    {
        return name.GetHash();
    }
};
