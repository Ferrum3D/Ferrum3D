﻿#pragma once
#include <EASTL/fixed_vector.h>
#include <FeCore/Base/Base.h>
#include <FeCore/DI/BaseDI.h>
#include <FeCore/Parallel/SpinLock.h>
#include <FeCore/Strings/Unicode.h>
#include <array>
#include <mutex>
#include <vector>

namespace FE::Memory
{
    //! @brief Type of global static allocator.
    enum class StaticAllocatorType : uint32_t
    {
        Default, //!< Default global heap allocator.
        Virtual, //!< Allocates virtual memory directly from the OS.
        Linear,  //!< Global linear allocator, the allocated memory will be freed only upon process termination.
    };
} // namespace FE::Memory


namespace FE::Env
{
    template<class T>
    class GlobalVariable;

    namespace Internal
    {
        // Internal interface, implemented only once, used internally in this namespace only.
        struct IEnvironment;
    } // namespace Internal


    bool IsDebuggerPresent();


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

        Name(const char* data, uint32_t length = 0)
            : Name(std::string_view{ data, length ? length : __builtin_strlen(data) })
        {
        }

        static bool TryGetExisting(std::string_view str, Name& result);

        [[nodiscard]] const Record* GetRecord() const;

        [[nodiscard]] bool Valid() const
        {
            return m_handle != kInvalidIndex;
        }

        [[nodiscard]] uint32_t size() const
        {
            return Valid() ? GetRecord()->m_size : 0;
        }

        [[nodiscard]] const char* c_str() const
        {
            return Valid() ? GetRecord()->m_data : nullptr;
        }

        explicit operator bool() const
        {
            return Valid();
        }

        friend bool operator==(Name lhs, Name rhs)
        {
            return lhs.m_handle == rhs.m_handle;
        }

        friend bool operator!=(Name lhs, Name rhs)
        {
            return lhs.m_handle != rhs.m_handle;
        }

        friend bool operator==(Name lhs, std::string_view rhs)
        {
            return lhs.GetRecord()->m_data == rhs;
        }

        friend bool operator!=(Name lhs, std::string_view rhs)
        {
            return lhs.GetRecord()->m_data != rhs;
        }

        friend bool operator==(std::string_view lhs, Name rhs)
        {
            return rhs == lhs;
        }

        friend bool operator!=(std::string_view lhs, Name rhs)
        {
            return rhs != lhs;
        }

    private:
        uint32_t m_handle = kInvalidIndex;
    };


    //! @brief Create a variable by unique name.
    //!
    //! Creates a global variable or finds an existing with the same identifier. Shared between different modules.
    //!
    //! @tparam T   - Type of variable to create.
    //! @param name - Unique name of variable to create.
    //!
    //! @return The created variable.
    template<class T, class... Args>
    GlobalVariable<T> CreateGlobalVariable(Name name, Args&&... args);

    //! @brief Create a variable by its type name.
    //!
    //! Creates a global variable or finds an existing with the same identifier. Shared between different modules.
    //!
    //! @tparam T    - Type of variable to create.
    //! @param value - Value to initialize the variable with.
    //!
    //! @return The created variable.
    template<class T, class... Args>
    GlobalVariable<T> CreateGlobalVariableByType(Args&&... args);

    //! @brief Allocate global variable storage.
    //!
    //! Creates a global variable by unique name, but doesn't initialize it.
    //!
    //! @tparam T   - Type of variable to allocate.
    //! @param name - Unique name of variable to allocate.
    //!
    //! @return The allocated variable.
    template<class T>
    GlobalVariable<T> AllocateGlobalVariable(Name name);

    //! @brief Find global variable by its name.
    //!
    //! Variable must be created by name before calling this function. Returns null if variable was not found.
    //!
    //! @tparam T   - Type of variable to find.
    //! @param name - Unique name of variable to find.
    template<class T>
    GlobalVariable<T> FindGlobalVariable(Name name);

    //! @brief Find global variable by its name.
    //!
    //! Variable must be created by type before calling this function. Returns null if variable was not found.
    //!
    //! @tparam T - Type of variable to find.
    template<class T>
    GlobalVariable<T> FindGlobalVariableByType();

    //! @brief Create global environment.
    //!
    //! @param allocator - Custom allocator.
    void CreateEnvironment();

    //! @brief Get global environment instance.
    //!
    //! Global environment must be created before any calls to this function. The
    //! environment won't be created automatically. This function will throw if the environment was not
    //! created and attached to this module.
    Internal::IEnvironment& GetEnvironment();

    //! @brief Attach an instance of global environment to this module.
    //!
    //! This function will most likely be called from another module that already have an environment attached
    //! or created.
    //!
    //! @param instance - Instance of global environment to attach.
    void AttachEnvironment(Internal::IEnvironment& instance);

    //! @brief Detach global environment.
    //!
    //! If the current module is the owner of global environment, it will be released, all variables will be
    //! deallocated and destructed. If the current module isn't the owner of environment, it will just set
    //! the module-local pointer to environment to `nullptr`.
    void DetachEnvironment();

    //! @brief Checks if the global environment exists and is attached to the current module.
    //!
    //! @return True if environment is attached.
    bool EnvironmentAttached();

    //! \internal
    namespace Internal
    {
        enum class VariableResultCode
        {
            NotFound,
            Created,
            Found,
            Removed
        };

        struct VariableResult final
        {
            void* pData = nullptr;
            VariableResultCode Code = VariableResultCode::NotFound;
        };

        //! @brief Environment interface.
        //!
        //! This is an internal-only interface that should not be exposed to engine users. Only one instance across
        //! all engine modules is allowed.
        struct IEnvironment
        {
            virtual ~IEnvironment() = default;

            //! @brief Find a global variable.
            //! \see FE::Env::FindGlobalVariable
            virtual VariableResult FindVariable(Name name) = 0;

            //! @brief Create a global variable.
            //! \see FE::Env::CreateGlobalVariable
            virtual VariableResult CreateVariable(Name name, size_t size, size_t alignment) = 0;

            //! @brief Remove a global variable.
            //! \see FE::Env::RemoveGlobalVariable
            virtual VariableResult RemoveVariable(Name name) = 0;

            //! @brief Destroy the environment.
            virtual void Destroy() = 0;
        };

        //! @brief Storage of global variable. Allocated once for variable.
        template<class T>
        class GlobalVariableStorage
        {
            std::aligned_storage_t<sizeof(T), alignof(T)> m_storage;
            Name m_name;
            uint32_t m_refCount;
            Threading::SpinLock m_mutex;
            bool m_isConstructed;

            friend class GlobalVariable<T>;

        public:
            //! @brief Create uninitialized global variable storage.
            //!
            //! @param name - Unique name of global variable.
            GlobalVariableStorage(Name name)
                : m_name(name)
                , m_refCount(0)
                , m_isConstructed(false)
            {
            }

            //! @brief Call variable's constructor.
            //!
            //! @param args - Arguments to call the constructor with.
            template<class... TArgs>
            void Construct(TArgs&&... args)
            {
                std::unique_lock lk(m_mutex);
                new (&m_storage) T(std::forward<TArgs>(args)...);
                m_isConstructed = true;
            }

            //! @brief Call variable's destructor. This function does _not_ deallocate memory.
            void Destruct()
            {
                std::unique_lock lk(m_mutex);
                reinterpret_cast<T*>(&m_storage)->~T();
                m_isConstructed = false;
            }

            //! @brief Check if variable was constructed.
            //!
            //! @return True if Construct was called.
            [[nodiscard]] bool IsConstructed() const
            {
                return m_isConstructed;
            }

            //! @brief Get underlying variable.
            const T* Get() const
            {
                return reinterpret_cast<const T*>(&m_storage);
            }

            //! @brief Add a reference to internal counter.
            void AddRef()
            {
                std::unique_lock lk(m_mutex);
                ++m_refCount;
            }

            //! \breif Remove a reference from internal counter.
            //!
            //! This function will call variable's destructor and deallocate memory if the reference counter reaches zero.
            void Release()
            {
                std::unique_lock lk(m_mutex);

                if (--m_refCount == 0)
                {
                    auto& env = FE::Env::GetEnvironment();
                    env.RemoveVariable(m_name);

                    if (m_isConstructed)
                        reinterpret_cast<T*>(&m_storage)->~T();

                    lk.unlock();
                    Memory::DefaultFree(this);
                }
            }
        };

        //! @brief Allocate GlobalVariableStorage for a global variable.
        //!
        //! @param name - The name of the variable.
        //!
        //! @return The allocated GlobalVariableStorage.
        template<class T>
        inline GlobalVariableStorage<T>* AllocateVariableStorage(Name name)
        {
            const size_t storageSize = sizeof(GlobalVariableStorage<T>);
            const size_t storageAlignment = alignof(GlobalVariableStorage<T>);
            auto [data, code] = GetEnvironment().CreateVariable(name, storageSize, storageAlignment);

            GlobalVariableStorage<T>* storage = nullptr;
            if (code == Internal::VariableResultCode::Created)
            {
                storage = new (data) GlobalVariableStorage<T>(name);
            }
            else if (code == Internal::VariableResultCode::Found)
            {
                storage = reinterpret_cast<GlobalVariableStorage<T>*>(data);
            }

            return storage;
        }

        //! \breif Attach an instance of global environment to current module.
        inline void AttachGlobalEnvironment(void* environmentPointer)
        {
            FE_CORE_ASSERT(!EnvironmentAttached(), "Attempt to attach second environment");
            FE::Env::AttachEnvironment(*static_cast<IEnvironment*>(environmentPointer));
        }

        //! @brief Implementation of FE::Env::CreateGlobalVariable
        template<class T, class... TArgs>
        inline GlobalVariable<T> CreateGlobalVariableImpl(Name name, TArgs&&... args)
        {
            Internal::GlobalVariableStorage<T>* storage = Internal::AllocateVariableStorage<T>(name);
            storage->Construct(std::forward<TArgs>(args)...);
            return FE::Env::GlobalVariable<T>(storage);
        }
    } // namespace Internal

    //! @brief Shared pointer to global variable storage.
    //!
    //! Size of this class is same as size of a pointer. It implements intrusive reference counted pointer.
    //!
    //! @tparam T Type of the variable that this class holds.
    template<class T>
    class GlobalVariable
    {
        Internal::GlobalVariableStorage<T>* m_storage;

    public:
        using StorageType = Internal::GlobalVariableStorage<T>;

        //! @brief Create a _null_ global variable.
        GlobalVariable()
            : m_storage(nullptr)
        {
        }

        explicit GlobalVariable(StorageType* storage)
            : m_storage(storage)
        {
            if (m_storage)
                m_storage->AddRef();
        }

        GlobalVariable(const GlobalVariable& other)
            : m_storage(other.m_storage)
        {
            if (m_storage)
                m_storage->AddRef();
        }

        GlobalVariable(GlobalVariable&& other) noexcept
            : m_storage(other.m_storage)
        {
            other.m_storage = nullptr;
        }

        GlobalVariable& operator=(const GlobalVariable& other)
        {
            GlobalVariable(other).Swap(*this);
            return *this;
        }

        GlobalVariable& operator=(GlobalVariable&& other) noexcept
        {
            GlobalVariable(std::move(other)).Swap(*this);
            return *this;
        }

        //! @brief Get name of the global variable.
        //!
        //! The name is either a normal string, which represents a user-defined unique variable name, or a special
        //! environment-generated name that starts with a '#'.
        //!
        //! @return The name of the variable.
        [[nodiscard]] Name GetName() const
        {
            return m_storage->m_name;
        }

        //! @brief Set this variable to _null_.
        void Reset()
        {
            GlobalVariable{}.Swap(*this);
        }

        ~GlobalVariable()
        {
            if (!EnvironmentAttached())
                return;
            if (m_storage)
                m_storage->Release();
        }

        //! @brief Check if the variable is not empty and is constructed (initialized).
        //!
        //! @return True if constructed.
        bool IsConstructed()
        {
            return m_storage && m_storage->IsConstructed();
        }

        //! @brief Swap contents of two variables.
        //!
        //! @param other - The variable to swap the content with.
        void Swap(GlobalVariable& other)
        {
            auto* t = other.m_storage;
            other.m_storage = m_storage;
            m_storage = t;
        }

        T* Get() const
        {
            FE_CORE_ASSERT(m_storage, "Global variable was empty");
            FE_CORE_ASSERT(m_storage->IsConstructed(), "Global variable was not constructed");
            return const_cast<T*>(m_storage->Get());
        }

        T& operator*() const
        {
            FE_CORE_ASSERT(m_storage, "Global variable was empty");
            FE_CORE_ASSERT(m_storage->IsConstructed(), "Global variable was not constructed");
            return *const_cast<T*>(m_storage->Get());
        }

        T* operator->() const
        {
            FE_CORE_ASSERT(m_storage, "Global variable was empty");
            FE_CORE_ASSERT(m_storage->IsConstructed(), "Global variable was not constructed");
            return const_cast<T*>(m_storage->Get());
        }

        explicit operator bool() const
        {
            return m_storage;
        }
    };

    //! @brief Compares names of two variables.
    template<class T>
    inline bool operator==(const GlobalVariable<T>& x, const GlobalVariable<T>& y)
    {
        return x.GetName() == y.GetName();
    }

    //! @brief Compares names of two variables.
    template<class T>
    inline bool operator!=(const GlobalVariable<T>& x, const GlobalVariable<T>& y)
    {
        return !(x == y);
    }

    std::pmr::memory_resource* GetStaticAllocator(Memory::StaticAllocatorType type);
    DI::IServiceProvider* GetServiceProvider();

    template<class T, class... Args>
    inline GlobalVariable<T> CreateGlobalVariable(Name name, Args&&... args)
    {
        const Name::Record* pRecord = name.GetRecord();
        const std::string_view nameView{ pRecord->m_data, pRecord->m_size };
        FE_CORE_ASSERT(nameView[0] != '#', "Names that start with a '#' are reserved for type name variables");
        return Internal::CreateGlobalVariableImpl<T, Args...>(name, std::forward<Args>(args)...);
    }

    template<class T, class... Args>
    inline GlobalVariable<T> CreateGlobalVariable(std::string_view name, Args&&... args)
    {
        FE_CORE_ASSERT(name[0] != '#', "Names that start with a '#' are reserved for type name variables");
        return Internal::CreateGlobalVariableImpl<T, Args...>(name, std::forward<Args>(args)...);
    }

    template<class T, class... Args>
    inline GlobalVariable<T> CreateGlobalVariableByType(Args&&... args)
    {
        std::string_view typeName = FE::TypeName<std::remove_pointer_t<T>>;
        eastl::fixed_vector<char, 64> vec;
        vec.reserve(static_cast<uint32_t>(typeName.length()) + 2);
        vec.push_back('#');
        for (char c : typeName)
            vec.push_back(c);
        vec.push_back('\0');

        const Name name{ vec.data(), vec.size() - 1 };
        return Internal::CreateGlobalVariableImpl<T>(name, std::forward<Args>(args)...);
    }

    template<class T>
    inline GlobalVariable<T> AllocateGlobalVariable(Name name)
    {
        return GlobalVariable<T>(Internal::AllocateVariableStorage<T>(name));
    }

    template<class T>
    inline GlobalVariable<T> FindGlobalVariable(Name name)
    {
        using StorageType = typename GlobalVariable<T>::StorageType;

        auto result = GetEnvironment().FindVariable(name);
        if (!result.pData)
            return {};

        return GlobalVariable<T>(static_cast<StorageType*>(result.pData));
    }

    template<class T>
    inline GlobalVariable<T> FindGlobalVariable(std::string_view name)
    {
        return FindGlobalVariable<T>(Name{ name });
    }

    template<class T>
    inline GlobalVariable<T> FindGlobalVariableByType()
    {
        std::string_view typeName = FE::TypeName<std::remove_pointer_t<T>>;
        eastl::fixed_vector<char, 64> vec;
        vec.reserve(static_cast<uint32_t>(typeName.length()) + 2);
        vec.push_back('#');
        for (char c : typeName)
            vec.push_back(c);
        vec.push_back('\0');

        const Name name{ vec.data(), vec.size() - 1 };
        return FindGlobalVariable<T>(name);
    }
} // namespace FE::Env


template<>
struct eastl::hash<FE::Env::Name>
{
    size_t operator()(FE::Env::Name name) const
    {
        return name.GetRecord()->m_hash;
    }
};
