#pragma once
#include <EASTL/variant.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Modules/Environment.h>
#include <festd/string.h>
#include <festd/vector.h>

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

        ConfigurationValue(const Name value)
            : m_type(ConfigurationValueType::kString)
        {
            SetImpl(value);
        }

        ConfigurationValue(const int64_t value)
            : m_type(ConfigurationValueType::kInt)
        {
            SetImpl(value);
        }

        ConfigurationValue(const double value)
            : m_type(ConfigurationValueType::kDouble)
        {
            SetImpl(value);
        }

        [[nodiscard]] ConfigurationValueType GetType() const
        {
            return m_type;
        }

        [[nodiscard]] Name AsName() const
        {
            CheckType(ConfigurationValueType::kString);
            return GetImpl<Name>();
        }

        [[nodiscard]] festd::string_view AsString() const
        {
            CheckType(ConfigurationValueType::kString);
            return festd::string_view{ GetImpl<Name>() };
        }

        [[nodiscard]] int64_t AsInt() const
        {
            CheckType(ConfigurationValueType::kInt);
            return GetImpl<int64_t>();
        }

        [[nodiscard]] double AsDouble() const
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
        [[nodiscard]] const T& GetImpl() const
        {
            return *reinterpret_cast<const T*>(&m_storage);
        }

        void CheckType(const ConfigurationValueType expected) const
        {
            FE_Assert(m_type == expected, "Type mismatch");
        }
    };


    struct ConfigurationSection final : public festd::intrusive_list_node
    {
        ConfigurationSection()
            : m_allocator(GetStaticAllocator(Memory::StaticAllocatorType::kLinear))
        {
        }

        ConfigurationSection(std::pmr::memory_resource* pAllocator)
            : m_allocator(pAllocator)
        {
        }

        [[nodiscard]] Name GetKey() const
        {
            return m_key;
        }

        [[nodiscard]] ConfigurationValue GetValue() const
        {
            return m_value;
        }

        [[nodiscard]] ConfigurationValue Get(const festd::string_view path) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return {};

            return section->m_value;
        }

        [[nodiscard]] Name GetName(const festd::string_view path, const Name defaultValue) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return defaultValue;

            return section->m_value.AsName();
        }

        [[nodiscard]] festd::string_view GetString(const festd::string_view path, const festd::string_view defaultValue) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return defaultValue;

            return section->m_value.AsString();
        }

        [[nodiscard]] int64_t GetInt(const festd::string_view path, const int64_t defaultValue) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return defaultValue;

            return section->m_value.AsInt();
        }

        [[nodiscard]] double GetDouble(const festd::string_view path, const double defaultValue) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return defaultValue;

            return section->m_value.AsDouble();
        }

        void Set(const festd::string_view path, const ConfigurationValue value)
        {
            ConfigurationSection* section = this;

            auto begin = path.begin();
            auto end = path.find_first_of('/');
            while (true)
            {
                const festd::string_view token{ begin, end };
                section = section->FindChild(token);
                if (section == nullptr)
                {
                    section = Memory::New<ConfigurationSection>(m_allocator, m_allocator);
                    m_children.push_back(*section);
                    section->m_key = Name{ token.data(), token.size() };
                }

                if (end == path.end())
                    break;

                begin = end;
                end = path.find_first_of(++end, '/');
            }

            section->m_value = value;
        }

        [[nodiscard]] const ConfigurationSection* GetSection(const festd::string_view path) const
        {
            const ConfigurationSection* section = this;

            auto begin = path.begin();
            auto end = path.find_first_of('/');
            while (true)
            {
                const festd::string_view token{ begin, end };
                section = section->FindChild(token);
                if (section == nullptr)
                    break;
                if (end == path.end())
                    break;

                begin = end;
                end = path.find_first_of(++end, '/');
            }

            return section;
        }

    private:
        // ConfigurationSection's destructor is never called, everything is allocated from a linear allocator.
        // All the memory is freed at once upon process termination.
        std::pmr::memory_resource* m_allocator;
        festd::intrusive_list<ConfigurationSection> m_children;
        Name m_key;
        ConfigurationValue m_value;

        ConfigurationSection* FindChild(const festd::string_view key)
        {
            Name keyName;
            if (!Name::TryGetExisting({ key.data(), key.size() }, keyName))
                return nullptr;

            for (ConfigurationSection& child : m_children)
            {
                if (child.m_key == keyName)
                    return &child;
            }

            return nullptr;
        }

        [[nodiscard]] const ConfigurationSection* FindChild(const festd::string_view key) const
        {
            Name keyName;
            if (!Name::TryGetExisting({ key.data(), key.size() }, keyName))
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

        Configuration(festd::span<const festd::string_view> commandLine);

        const ConfigurationSection* GetSection(const festd::string_view path) const
        {
            std::lock_guard lk{ m_lock };
            for (const ConfigurationSection& section : m_rootSections)
            {
                if (const ConfigurationSection* pSection = section.GetSection(path))
                    return pSection;
            }

            return nullptr;
        }

        ConfigurationValue Get(const festd::string_view path) const
        {
            std::lock_guard lk{ m_lock };
            for (const ConfigurationSection& section : m_rootSections)
            {
                if (const ConfigurationValue value = section.Get(path))
                    return value;
            }

            return {};
        }

        Name GetName(const festd::string_view path, const Name defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsName();
        }

        festd::string_view GetString(const festd::string_view path, const festd::string_view defaultValue) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return defaultValue;

            return section->GetValue().AsString();
        }

        int64_t GetInt(const festd::string_view path, const int64_t defaultValue) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return defaultValue;

            return section->GetValue().AsInt();
        }

        double GetDouble(const festd::string_view path, const double defaultValue) const
        {
            const ConfigurationSection* section = GetSection(path);
            if (section == nullptr)
                return defaultValue;

            return section->GetValue().AsDouble();
        }

    private:
        static constexpr uint32_t kSectionCount = 2;

        festd::fixed_vector<ConfigurationSection, kSectionCount> m_rootSections;
        mutable Threading::SpinLock m_lock;
    };
} // namespace FE::Env
