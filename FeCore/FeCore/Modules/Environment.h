#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/IBasicAllocator.h>
#include <FeCore/Strings/FeUnicode.h>
#include <FeCore/Utils/Result.h>
#include <array>
#include <mutex>
#include <vector>

namespace FE::Env
{
    template<class T>
    class GlobalVariable;

    namespace Internal
    {
        // Internal interface, implemented only once, used internally in this namespace only.
        class IEnvironment;
    } // namespace Internal

    //! \brief Create a variable by unique name.
    //!
    //! Creates a global variable or finds an existing with the same identifier. Shared between different modules.
    //!
    //! \tparam T        - Type of variable to create.
    //! \param [in] name - Unique name of variable to create.
    //!
    //! \return The created variable.
    template<class T, class... Args>
    GlobalVariable<T> CreateGlobalVariable(std::string_view name, Args&&... args);

    //! \brief Create a variable by its type name.
    //!
    //! Creates a global variable or finds an existing with the same identifier. Shared between different modules.
    //!
    //! \tparam T         - Type of variable to create.
    //! \param [in] value - Value to initialize the variable with.
    //!
    //! \return The created variable.
    template<class T, class... Args>
    GlobalVariable<T> CreateGlobalVariableByType(Args&&... args);

    //! \brief Allocate global variable storage.
    //!
    //! Creates a global variable by unique ID, but doesn't initilize it.
    //!
    //! \tparam T      - Type of variable to allocate.
    //! \param [in] id - Unique ID of variable to allocate.
    //!
    //! \return The allocated variable.
    template<class T>
    GlobalVariable<T> AllocateGlobalVariable(UInt32 id);

    //! \brief Allocate global variable storage.
    //!
    //! Creates a global variable by unique name, but doesn't initilize it.
    //!
    //! \tparam T        - Type of variable to allocate.
    //! \param [in] name - Unique name of variable to allocate.
    //!
    //! \return The allocated variable.
    template<class T>
    GlobalVariable<T> AllocateGlobalVariable(std::string_view name);

    //! \brief Find global variable by its name.
    //!
    //! Variable must be created by name before calling this function. Returns an Error result if variable was not found.
    //!
    //! \tparam T        - Type of variable to find.
    //! \param [in] name - Unique name of variable to find.
    //! \return The result that can contain the variable.
    template<class T>
    Result<GlobalVariable<T>> FindGlobalVariable(std::string_view name);

    //! \brief Find global variable by its name.
    //!
    //! Variable must be created by type before calling this function. Returns an Error result if variable was not found.
    //!
    //! \tparam T - Type of variable to find.
    //! \return The result that can contain the variable.
    template<class T>
    Result<GlobalVariable<T>> FindGlobalVariableByType();

    //! \brief Create global environment.
    //!
    //! A custom allocator can be provided. If the provided allocator was `nullptr`, aligned versions
    //! of `malloc()` and `free()` will be used to allocate storage for global variables.
    //!
    //! \param [in] allocator - Custom allocator.
    void CreateEnvironment(IBasicAllocator* allocator = nullptr);

    //! \brief Get global environment instance.
    //!
    //! Global environment must be created before any calls to this function. Unlike other singletons, the
    //! environment won't be created automatically. This function will throw if the environment was not
    //! created and attached to this module.
    Internal::IEnvironment& GetEnvironment();

    //! \brief Attach an instance of global environment to this module.
    //!
    //! This function will most likely be called from another module that already have an environment attached
    //! or created.
    //!
    //! \param [in] instance - Instance of global environment to attach.
    void AttachEnvironment(Internal::IEnvironment& instance);

    //! \brief Detach global environment.
    //!
    //! If the current module is the owner of global environment, it will be released, all variables will be
    //! deallocated and destructed. If the current module isn't the owner of environment, it will just set
    //! the module-local pointer to environment to `nullptr`.
    void DetachEnvironment();

    //! \brief Checks if the global environment exists and is attached to the current module.
    //!
    //! \return True if environment is attached.
    bool EnvironmentAttached();

    //! \internal
    namespace Internal
    {
        //! \brief Type of error returned by environment functions.
        enum class VariableError : UInt8
        {
            NotFound,       //!< Variable was not found.
            AllocationError //!< Couldn't allocate variable because of an allocation error.
                            //!< This is typically a fatal unrecoverable error, but some allocators can collect garbage.
        };

        //! \brief Type of success result returned by environment functions.
        enum class VariableOk : UInt8
        {
            Created,
            Found,
            Removed
        };

        //! \brief Specialization of \ref Result that uses \ref VaiableError and \ref VariableOk
        using VariableResult = Result<void*, VariableError, VariableOk>;

        //! \brief Environment interface.
        //!
        //! This is an internal-only interface that should not be exposed to engine users. Only one instance across
        //! all engine modules is allowed.
        class IEnvironment
        {
        public:
            virtual ~IEnvironment() = default;

            //! \brief Find a global variable.
            //! \see FE::Env::FindGlobalVariable
            virtual VariableResult FindVariable(std::string_view name) = 0;

            //! \brief Create a global variable.
            //! \see FE::Env::CreateGlobalVariable
            virtual VariableResult CreateVariable(
                std::vector<char>&& name, size_t size, size_t alignment, std::string_view& nameView) = 0;

            //! \brief Remove a global variable.
            //! \see FE::Env::RemoveGlobalVariable
            virtual VariableResult RemoveVariable(std::string_view name) = 0;

            //! \brief Get the allocator currently used by the environment.
            virtual FE::IBasicAllocator* GetAllocator() = 0;

            //! \brief Destroy the environment.
            virtual void Destroy() = 0;
        };

        //! \brief Storage of global variable. Allocated once for variable.
        template<class T>
        class GlobalVariableStorage
        {
            std::string_view m_Name;
            UInt32 m_RefCount;
            std::aligned_storage_t<sizeof(T), alignof(T)> m_Storage;
            std::mutex m_Mutex;
            bool m_IsConstructed;

            friend class GlobalVariable<T>;

        public:
            //! \brief Create uninitialized global variable storage.
            //!
            //! \param [in] name - Unique name of global variable.
            inline GlobalVariableStorage(std::string_view name)
                : m_Name(name)
                , m_RefCount(0)
                , m_IsConstructed(false)
            {
            }

            //! \brief Call variable's constructor.
            //!
            //! \tparam Args     - Types of arguments to call the constructor with.
            //! \param [in] args - Arguments to call the constructor with.
            template<class... Args>
            inline void Construct(Args&&... args)
            {
                std::unique_lock lk(m_Mutex);
                new (&m_Storage) T(std::forward<Args>(args)...);
                m_IsConstructed = true;
            }

            //! \brief Call variable's destructor. This function does _not_ deallocate memory.
            inline void Destruct()
            {
                std::unique_lock lk(m_Mutex);
                reinterpret_cast<T*>(&m_Storage)->~T();
                m_IsConstructed = false;
            }

            //! \brief Check if variable was constructed.
            //!
            //! \return True if \ref Construct was called.
            inline bool IsConstructed() const
            {
                return m_IsConstructed;
            }

            //! \brief Get underlying variable.
            inline T& Get()
            {
                return *reinterpret_cast<T*>(&m_Storage);
            }

            //! \brief Get underlying variable.
            inline const T& Get() const
            {
                return *reinterpret_cast<const T*>(&m_Storage);
            }

            //! \brief Add a reference to internal counter.
            inline void AddRef()
            {
                std::unique_lock lk(m_Mutex);
                ++m_RefCount;
            }

            //! \breif Remove a reference from internal counter.
            //!
            //! This function will call variable's destructor and deallocate memory if the reference counter reaches zero.
            inline void Release()
            {
                std::unique_lock<std::mutex> lk(m_Mutex);

                if (--m_RefCount == 0)
                {
                    auto& env       = FE::Env::GetEnvironment();
                    auto* allocator = env.GetAllocator();
                    env.RemoveVariable(m_Name);

                    if (m_IsConstructed)
                        reinterpret_cast<T*>(&m_Storage)->~T();

                    lk.unlock();
                    allocator->Deallocate(this);
                }
            }
        };

        //! \brief Allocate \ref GlobalVariableStorage for a global variable.
        //!
        //! \param [in] name - A vector of characters that represents the name of the variable.
        //!
        //! \return The allocated \ref GlobalVariableStorage.
        template<class T>
        inline GlobalVariableStorage<T>* AllocateVariableStorage(std::vector<char>&& name)
        {
            std::string_view nameView;
            auto [ok, data] =
                GetEnvironment()
                    .CreateVariable(
                        std::move(name), sizeof(GlobalVariableStorage<T>), alignof(GlobalVariableStorage<T>), nameView)
                    .ExpectEx("Couldn't create variable");

            GlobalVariableStorage<T>* storage = nullptr;
            if (ok == Internal::VariableOk::Created)
                storage = new (data) GlobalVariableStorage<T>(nameView);
            else if (ok == Internal::VariableOk::Found)
                storage = reinterpret_cast<GlobalVariableStorage<T>*>(data);

            return storage;
        }

        //! \breif Attach an instance of global environment to current module.
        inline void AttachGlobalEnvironment(void* environmentPointer)
        {
            FE_CORE_ASSERT(!EnvironmentAttached(), "Attempt to attach second environment");
            FE::Env::AttachEnvironment(*static_cast<IEnvironment*>(environmentPointer));
        }

        //! \brief Implementation of \ref FE::Env::CreateGlobalVariable
        template<class T, class... Args>
        inline GlobalVariable<T> CreateGlobalVariableImpl(std::vector<char>&& name, Args&&... args)
        {
            Internal::GlobalVariableStorage<T>* storage = Internal::AllocateVariableStorage<T>(std::move(name));

            storage->Construct(std::forward<Args>(args)...);

            return storage;
        }
    } // namespace Internal

    //! \brief Shared pointer to global variable storage.
    //!
    //! Size of this class is same as size of a pointer. It implements intrusive reference counted pointer.
    //!
    //! \tparam T Type of the variable that this class holds.
    template<class T>
    class GlobalVariable
    {
        Internal::GlobalVariableStorage<T>* m_Storage;

    public:
        using StorageType = Internal::GlobalVariableStorage<T>;

        //! \brief Create a _null_ global variable.
        inline GlobalVariable()
            : m_Storage(nullptr)
        {
        }

        inline GlobalVariable(Internal::GlobalVariableStorage<T>* storage)
            : m_Storage(storage)
        {
            if (m_Storage)
                m_Storage->AddRef();
        }

        inline GlobalVariable(const GlobalVariable& other)
            : m_Storage(other.m_Storage)
        {
            if (m_Storage)
                m_Storage->AddRef();
        }

        inline GlobalVariable(GlobalVariable&& other) noexcept
            : m_Storage(other.m_Storage)
        {
            other.m_Storage = nullptr;
        }

        inline GlobalVariable& operator=(const GlobalVariable& other)
        {
            GlobalVariable(other).Swap(*this);
            return *this;
        }

        inline GlobalVariable& operator=(GlobalVariable&& other) noexcept
        {
            GlobalVariable(std::move(other)).Swap(*this);
            return *this;
        }

        //! \brief Get name of the global variable.
        //!
        //! The name is either a normal string, which represents a user-defined unique variable name, or a special
        //! environment-generated name that starts from a '#'.
        //!
        //! \return The name of the variable.
        inline std::string_view GetName() const
        {
            return m_Storage->m_Name;
        }

        //! \brief Set this variable to _null_.
        inline void Reset()
        {
            GlobalVariable{}.Swap(*this);
        }

        inline ~GlobalVariable()
        {
            if (!EnvironmentAttached())
                return;
            if (m_Storage)
                m_Storage->Release();
        }

        //! \brief Check if the variable is not empty and is constructed (initialized).
        //!
        //! \return True if constructed.
        inline bool IsConstructed()
        {
            return m_Storage && m_Storage->IsConstructed();
        }

        //! \brief Swap contents of two variables.
        //!
        //! \param [in] other - The variable to swap the content with.
        inline void Swap(GlobalVariable& other)
        {
            auto* t         = other.m_Storage;
            other.m_Storage = m_Storage;
            m_Storage       = t;
        }

        inline T& operator*()
        {
            FE_CORE_ASSERT(m_Storage, "Global variable was empty");
            FE_CORE_ASSERT(m_Storage->IsConstructed(), "Global variable was not constructed");
            return m_Storage->Get();
        }

        inline T* operator->()
        {
            FE_CORE_ASSERT(m_Storage, "Global variable was empty");
            FE_CORE_ASSERT(m_Storage->IsConstructed(), "Global variable was not constructed");
            return &m_Storage->Get();
        }

        inline const T& operator*() const
        {
            FE_CORE_ASSERT(m_Storage, "Global variable was empty");
            FE_CORE_ASSERT(m_Storage->IsConstructed(), "Global variable was not constructed");
            return m_Storage->Get();
        }

        inline const T* operator->() const
        {
            FE_CORE_ASSERT(m_Storage, "Global variable was empty");
            FE_CORE_ASSERT(m_Storage->IsConstructed(), "Global variable was not constructed");
            return m_Storage->Get();
        }

        inline operator bool() const
        {
            return m_Storage;
        }
    };

    //! \brief Compares names of two variables.
    template<class T>
    inline bool operator==(const GlobalVariable<T>& x, const GlobalVariable<T>& y)
    {
        return x.GetName() == y.GetName();
    }

    //! \brief Compares names of two variables.
    template<class T>
    inline bool operator!=(const GlobalVariable<T>& x, const GlobalVariable<T>& y)
    {
        return !(x == y);
    }

    template<class T, class... Args>
    inline GlobalVariable<T> CreateGlobalVariable(std::string_view name, Args&&... args)
    {
        FE_CORE_ASSERT(name[0] != '#', "Names that start with a '#' are reserved for type name and ID variables");
        std::vector<char> str;
        str.reserve(name.length());
        for (char c : name)
            str.push_back(c);
        return Internal::CreateGlobalVariableImpl<T, Args...>(std::move(str), std::forward<Args>(args)...);
    }

    template<class T, class... Args>
    inline GlobalVariable<T> CreateGlobalVariableByType(Args&&... args)
    {
        std::string_view typeName = FE::TypeName<std::remove_pointer_t<T>>();
        std::vector<char> str;
        str.reserve(typeName.length() + 1);
        str.push_back('#');
        for (char c : typeName)
            str.push_back(c);
        return Internal::CreateGlobalVariableImpl<T>(std::move(str), std::forward<Args>(args)...);
    }

    template<class T>
    inline GlobalVariable<T> AllocateGlobalVariable(UInt32 id)
    {
        std::vector<char> str;
        str.reserve(9);
        str.push_back('#');
        for (int i = 7; i >= 0; --i)
            str.push_back(FE::IntToHexChar((id >> i * 4) & 0xF));
        return Internal::AllocateVariableStorage<T>(std::move(str));
    }

    template<class T>
    inline GlobalVariable<T> AllocateGlobalVariable(std::string_view name)
    {
        std::vector<char> str;
        str.reserve(name.length());
        for (char c : name)
            str.push_back(c);
        return Internal::AllocateVariableStorage<T>(std::move(str));
    }

    template<class T>
    inline Result<GlobalVariable<T>> FindGlobalVariable(std::string_view name)
    {
        using Var         = GlobalVariable<T>;
        using StorageType = typename GlobalVariable<T>::StorageType;

        return GetEnvironment().FindVariable(name).Match(
            [&](const Internal::VariableOk&, void* ptr) {
                Var variable(reinterpret_cast<StorageType*>(ptr));
                return ptr == nullptr ? Result<Var>::Err() : Result<Var>::Ok(std::move(variable));
            },
            [&](const Internal::VariableError&) {
                return Result<Var>::Err();
            });
    }

    template<class T>
    inline Result<GlobalVariable<T>> FindGlobalVariableByType()
    {
        auto typeName = TypeName<std::remove_pointer_t<T>>();
        std::vector<char> str{};
        str.resize(typeName.length() + 1);
        size_t size = 0;
        str[size++] = '#';
        for (char c : typeName)
            str[size++] = c;

        return FindGlobalVariable<T>(std::string_view(str.data(), str.size()));
    }
} // namespace FE::Env
