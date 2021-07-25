#pragma once
#include <vector>
#include <array>
#include <Memory/IBasicAllocator.h>
#include <Utils/CoreUtils.h>
#include <Utils/Result.h>

namespace FE::Env
{
    template<class T>
    class GlobalVariable;

    namespace Internal
    {
        // Internal interface, implemented only once, used internally in this namespace only
        class IEnvironment;
    } // namespace Internal

    template<class T, class... Args>
    GlobalVariable<T> CreateGlobalVariable(uint32_t id, Args&&... args);

    template<class T, class... Args>
    GlobalVariable<T> CreateGlobalVariable(std::string_view name, Args&&... args);

    template<class T>
    GlobalVariable<T*> CreateGlobalVariableByType(T* value);

    template<class T>
    GlobalVariable<T> AllocateGlobalVariable(uint32_t id);

    template<class T>
    GlobalVariable<T> AllocateGlobalVariable(std::string_view name);

    template<class T>
    Result<GlobalVariable<T>> FindGlobalVariable(uint32_t id);

    template<class T>
    Result<GlobalVariable<T>> FindGlobalVariable(std::string_view name);

    template<class T>
    Result<GlobalVariable<T*>> FindGlobalVariableByType();

    FE_CORE_API void CreateEnvironment(IBasicAllocator* allocator = nullptr);

    FE_CORE_API Internal::IEnvironment& GetEnvironment();

    FE_CORE_API void AttachEnvironment(Internal::IEnvironment& instance);

    FE_CORE_API void DetachEnvironment();

    FE_CORE_API bool EnvironmentAttached();

    namespace Internal
    {
        enum class VariableError : uint8_t
        {
            NotFound,
            AllocationError
        };

        enum class VariableOk : uint8_t
        {
            Created,
            Found,
            Removed
        };

        using VariableResult = Result<void*, VariableError, VariableOk>;

        class IEnvironment
        {
        public:
            virtual ~IEnvironment() {}

            virtual VariableResult FindVariable(std::string_view name) = 0;

            virtual VariableResult CreateVariable(std::vector<char>&& name, size_t size, size_t alignment, std::string_view& nameView) = 0;

            virtual VariableResult RemoveVariable(std::string_view name) = 0;

            virtual FE::IBasicAllocator* GetAllocator() = 0;

            virtual void Destroy() = 0;
        };

        template<class T>
        class GlobalVariableStorage
        {
            std::string_view m_Name;
            uint32_t m_RefCount;
            typename std::aligned_storage<sizeof(T), alignof(T)>::type m_Storage;
            std::mutex m_Mutex;
            bool m_IsConstructed;

            friend class GlobalVariable<T>;

        public:
            inline GlobalVariableStorage(std::string_view name)
                : m_Name(name)
                , m_RefCount(0)
                , m_IsConstructed(false)
            {
            }

            template<class... Args>
            inline void Construct(Args&&... args)
            {
                std::unique_lock lk(m_Mutex);
                new (&m_Storage) T(std::forward<Args>(args)...);
                m_IsConstructed = true;
            }

            inline void Destruct()
            {
                std::unique_lock lk(m_Mutex);
                reinterpret_cast<T*>(&m_Storage)->~T();
                m_IsConstructed = false;
            }

            inline bool IsConstructed() const
            {
                return m_IsConstructed;
            }

            inline T& Get()
            {
                return *reinterpret_cast<T*>(&m_Storage);
            }

            inline const T& Get() const
            {
                return *reinterpret_cast<const T*>(&m_Storage);
            }

            inline void AddRef()
            {
                std::unique_lock lk(m_Mutex);
                ++m_RefCount;
            }

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

        template<class T>
        inline GlobalVariableStorage<T>* AllocateVariableStorage(std::vector<char>&& name)
        {
            std::string_view nameView;
            auto [ok, data] =
                GetEnvironment()
                    .CreateVariable(std::move(name), sizeof(GlobalVariableStorage<T>), alignof(GlobalVariableStorage<T>), nameView)
                    .ExpectEx("Couldn't create variable");

            GlobalVariableStorage<T>* storage = nullptr;
            if (ok == Internal::VariableOk::Created)
                storage = new (data) GlobalVariableStorage<T>(nameView);
            else if (ok == Internal::VariableOk::Found)
                storage = reinterpret_cast<GlobalVariableStorage<T>*>(data);

            return storage;
        }

        inline void AttachGlobalEnvironment(void* environmentPointer)
        {
            FE_ASSERT_MSG(!EnvironmentAttached(), "Attempt to attach second environment");
            FE::Env::AttachEnvironment(*static_cast<IEnvironment*>(environmentPointer));
        }

        template<class T, class... Args>
        inline GlobalVariable<T> CreateGlobalVariableImpl(std::vector<char>&& name, Args&&... args)
        {
            Internal::GlobalVariableStorage<T>* storage = Internal::AllocateVariableStorage<T>(std::move(name));

            storage->Construct(std::forward<Args>(args)...);

            return storage;
        }
    } // namespace Internal

    template<class T>
    class GlobalVariable
    {
        Internal::GlobalVariableStorage<T>* m_Storage;

    public:
        using StorageType = Internal::GlobalVariableStorage<T>;

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

        inline GlobalVariable(GlobalVariable&& other)
            : m_Storage(other.m_Storage)
        {
            other.m_Storage = nullptr;
        }

        inline GlobalVariable& operator=(const GlobalVariable& other)
        {
            GlobalVariable(other).Swap(*this);
            return *this;
        }

        inline GlobalVariable& operator=(GlobalVariable&& other)
        {
            GlobalVariable(std::move(other)).Swap(*this);
            return *this;
        }

        inline std::string_view GetName() const
        {
            return m_Storage->m_Name;
        }

        inline void Reset()
        {
            GlobalVariable{}.Swap(*this);
        }

        inline ~GlobalVariable()
        {
            if (m_Storage)
                m_Storage->Release();
        }

        inline bool IsConstructed()
        {
            return m_Storage && m_Storage->IsConstructed();
        }

        inline void Swap(GlobalVariable& other)
        {
            auto* t         = other.m_Storage;
            other.m_Storage = m_Storage;
            m_Storage       = t;
        }

        inline T& operator*()
        {
            FE_ASSERT_MSG(m_Storage, "Global variable was empty");
            FE_ASSERT_MSG(m_Storage->IsConstructed(), "Global variable was not constructed");
            return m_Storage->Get();
        }

        inline T* operator->()
        {
            FE_ASSERT_MSG(m_Storage, "Global variable was empty");
            FE_ASSERT_MSG(m_Storage->IsConstructed(), "Global variable was not constructed");
            return m_Storage->Get();
        }

        inline const T& operator*() const
        {
            FE_ASSERT_MSG(m_Storage, "Global variable was empty");
            FE_ASSERT_MSG(m_Storage->IsConstructed(), "Global variable was not constructed");
            return m_Storage->Get();
        }

        inline const T* operator->() const
        {
            FE_ASSERT_MSG(m_Storage, "Global variable was empty");
            FE_ASSERT_MSG(m_Storage->IsConstructed(), "Global variable was not constructed");
            return m_Storage->Get();
        }

        inline operator bool() const
        {
            return m_Storage;
        }
    };

    template<class T>
    inline bool operator==(const GlobalVariable<T>& x, const GlobalVariable<T>& y)
    {
        return x.GetName() == y.GetName();
    }

    template<class T>
    inline bool operator!=(const GlobalVariable<T>& x, const GlobalVariable<T>& y)
    {
        return !(x == y);
    }

    template<class T, class... Args>
    inline GlobalVariable<T> CreateGlobalVariable(std::string_view name, Args&&... args)
    {
        FE_ASSERT_MSG(name[0] != '#', "Names that start with a '#' are reserved for type name and ID variables");
        std::vector<char> str;
        str.reserve(name.length());
        for (char c : name)
            str.push_back(c);
        return Internal::CreateGlobalVariableImpl<T, Args...>(std::move(str), std::forward<Args>(args)...);
    }

    template<class T, class... Args>
    inline GlobalVariable<T> CreateGlobalVariable(uint32_t id, Args&&... args)
    {
        std::vector<char> str;
        str.reserve(9);
        str.push_back('#');
        for (int i = 7; i >= 0; --i)
            str.push_back(FE::IntToHexChar((id >> i * 4) & 0xF));
        return Internal::CreateGlobalVariableImpl<T, Args...>(std::move(str), std::forward<Args>(args)...);
    }

    template<class T>
    GlobalVariable<T*> CreateGlobalVariableByType(T* value)
    {
        std::string_view typeName = FE::TypeName<T>();
        std::vector<char> str;
        str.reserve(typeName.length() + 1);
        str.push_back('#');
        for (char c : typeName)
            str.push_back(c);
        return Internal::CreateGlobalVariableImpl<T*>(std::move(str), value);
    }

    template<class T>
    inline GlobalVariable<T> AllocateGlobalVariable(uint32_t id)
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
    inline Result<GlobalVariable<T>> FindGlobalVariable(uint32_t id)
    {
        std::array<char, 9> str;
        size_t size = 0;
        str[size++] = '#';
        for (int i = 7; i >= 0; --i)
            str[size++] = (FE::IntToHexChar((id >> i * 4) & 0xF));

        return FindGlobalVariable<T>(std::string_view(str.data(), str.size()));
    }

    template<class T>
    Result<GlobalVariable<T*>> FindGlobalVariableByType()
    {
        std::array<char, TypeName<T>().length() + 1> str;
        size_t size = 0;
        str[size++] = '#';
        for (char c : TypeName<T>())
            str[size++] = c;

        return FindGlobalVariable<T*>(std::string_view(str.data(), str.size()));
    }
} // namespace FE::Env
