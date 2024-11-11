#pragma once
#include <EASTL/variant.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE::Env
{
    enum class ConfigurationValueType : uint32_t
    {
        kNull,
        kString,
        kInt,
        kDouble,
    };


    struct ConfigurationValue final
    {
        ConfigurationValue() = default;

        ConfigurationValue(Name value)
            : m_type(ConfigurationValueType::kString)
        {
            SetImpl(value);
        }

        ConfigurationValue(int64_t value)
            : m_type(ConfigurationValueType::kInt)
        {
            SetImpl(value);
        }

        ConfigurationValue(double value)
            : m_type(ConfigurationValueType::kDouble)
        {
            SetImpl(value);
        }

        ConfigurationValueType GetType() const
        {
            return m_type;
        }

        const Name& AsName() const
        {
            CheckType(ConfigurationValueType::kString);
            return GetImpl<Name>();
        }

        StringSlice AsString() const
        {
            CheckType(ConfigurationValueType::kString);
            return GetImpl<Name>();
        }

        int64_t AsInt() const
        {
            CheckType(ConfigurationValueType::kInt);
            return GetImpl<int64_t>();
        }

        double AsDouble() const
        {
            CheckType(ConfigurationValueType::kDouble);
            return GetImpl<double>();
        }

        explicit operator bool() const
        {
            return m_type != ConfigurationValueType::kNull;
        }

    private:
        uint64_t m_storage = 0;
        ConfigurationValueType m_type = ConfigurationValueType::kNull;

        template<class T>
        void SetImpl(const T& value)
        {
            if constexpr (sizeof(T) < sizeof(m_storage))
                m_storage = 0;

            memcpy(&m_storage, &value, sizeof(T));
        }

        template<class T>
        const T& GetImpl() const
        {
            return *reinterpret_cast<const T*>(&m_storage);
        }

        void CheckType(ConfigurationValueType expected) const
        {
            FE_AssertMsg(m_type == expected, "Type mismatch");
        }
    };


    struct ConfigurationSection final : public festd::intrusive_list_node
    {
        ConfigurationSection()
            : m_allocator(Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear))
        {
        }

        ConfigurationSection(std::pmr::memory_resource* pAllocator)
            : m_allocator(pAllocator)
        {
        }

        Name GetKey() const
        {
            return m_key;
        }

        ConfigurationValue GetValue() const
        {
            return m_value;
        }

        ConfigurationValue Get(StringSlice path) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return {};

            return pSection->m_value;
        }

        Env::Name GetName(StringSlice path, Env::Name defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_value.AsName();
        }

        StringSlice GetString(StringSlice path, StringSlice defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_value.AsString();
        }

        int64_t GetInt(StringSlice path, int64_t defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_value.AsInt();
        }

        double GetDouble(StringSlice path, double defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_value.AsDouble();
        }

        void Set(StringSlice path, ConfigurationValue value)
        {
            ConfigurationSection* pSection = this;

            auto begin = path.begin();
            auto end = path.FindFirstOf('/');
            while (true)
            {
                const StringSlice token{ begin, end };
                pSection = pSection->FindChild(token);
                if (pSection == nullptr)
                {
                    pSection = Memory::New<ConfigurationSection>(m_allocator, m_allocator);
                    m_children.push_back(*pSection);
                    pSection->m_key = Name{ token.Data(), static_cast<uint32_t>(token.Size()) };
                }

                if (end == path.end())
                    break;

                begin = end;
                end = path.FindFirstOf(++end, '/');
            }

            pSection->m_value = value;
        }

        const ConfigurationSection* GetSection(StringSlice path) const
        {
            const ConfigurationSection* pSection = this;

            auto begin = path.begin();
            auto end = path.FindFirstOf('/');
            while (true)
            {
                const StringSlice token{ begin, end };
                pSection = pSection->FindChild(token);
                if (pSection == nullptr)
                    break;
                if (end == path.end())
                    break;

                begin = end;
                end = path.FindFirstOf(++end, '/');
            }

            return pSection;
        }

    private:
        // ConfigurationSection's destructor is never called, everything is allocated from a linear allocator.
        // All the memory is freed at once upon process termination.
        std::pmr::memory_resource* m_allocator;
        festd::intrusive_list<ConfigurationSection> m_children;
        Name m_key;
        ConfigurationValue m_value;

        ConfigurationSection* FindChild(StringSlice key)
        {
            Name keyName;
            if (!Name::TryGetExisting({ key.Data(), key.Size() }, keyName))
                return nullptr;

            for (ConfigurationSection& child : m_children)
            {
                if (child.m_key == keyName)
                    return &child;
            }

            return nullptr;
        }

        const ConfigurationSection* FindChild(StringSlice key) const
        {
            Name keyName;
            if (!Name::TryGetExisting({ key.Data(), key.Size() }, keyName))
                return nullptr;

            for (const ConfigurationSection& child : m_children)
            {
                if (child.m_key == keyName)
                    return &child;
            }

            return nullptr;
        }
    };


    //! @brief Engine configuration root.
    //!
    //! There are two ways to provide configuration:
    //! - Command line arguments:
    //!    -c path/to/key1=123 -c "path/to/key2=my string"
    //!
    //! - ferrum-config.json:
    //! \code{.cpp}
    //!     {
    //!         "path": {
    //!             "to": {
    //!                 "key1" : 123,
    //!                 "key2" : "my string"
    //!             }
    //!         }
    //!     }
    //! \endcode
    struct Configuration final : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(Configuration, "FE804C2B-6671-47E7-8A96-4CD59680CCE4");

        Configuration(festd::span<const StringSlice> commandLine);

        const ConfigurationSection* GetSection(StringSlice path) const
        {
            std::lock_guard lk{ m_lock };
            for (const ConfigurationSection& section : m_rootSections)
            {
                if (const ConfigurationSection* pSection = section.GetSection(path))
                    return pSection;
            }

            return nullptr;
        }

        ConfigurationValue Get(StringSlice path) const
        {
            std::lock_guard lk{ m_lock };
            for (const ConfigurationSection& section : m_rootSections)
            {
                if (const ConfigurationValue value = section.Get(path))
                    return value;
            }

            return {};
        }

        Env::Name GetName(StringSlice path, Env::Name defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsName();
        }

        StringSlice GetString(StringSlice path, StringSlice defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsString();
        }

        int64_t GetInt(StringSlice path, int64_t defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsInt();
        }

        double GetDouble(StringSlice path, double defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsDouble();
        }

    private:
        static constexpr uint32_t kSectionCount = 2;

        festd::fixed_vector<ConfigurationSection, kSectionCount> m_rootSections;
        mutable SpinLock m_lock;
    };
} // namespace FE::Env
