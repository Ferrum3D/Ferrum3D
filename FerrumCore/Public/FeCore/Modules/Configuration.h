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
        Null,
        String,
        Int,
        Double,
    };


    class ConfigurationValue final
    {
        uint64_t m_Storage = 0;
        ConfigurationValueType m_Type = ConfigurationValueType::Null;

        template<class T>
        inline void SetImpl(const T& value)
        {
            if constexpr (sizeof(T) < sizeof(m_Storage))
                m_Storage = 0;

            memcpy(&m_Storage, &value, sizeof(T));
        }

        template<class T>
        inline const T& GetImpl() const
        {
            return *reinterpret_cast<const T*>(&m_Storage);
        }

        inline void CheckType(ConfigurationValueType expected) const
        {
            FE_AssertMsg(m_Type == expected, "Type mismatch");
        }

    public:
        inline ConfigurationValue() = default;

        inline ConfigurationValue(Name value)
            : m_Type(ConfigurationValueType::String)
        {
            SetImpl(value);
        }

        inline ConfigurationValue(int64_t value)
            : m_Type(ConfigurationValueType::Int)
        {
            SetImpl(value);
        }

        inline ConfigurationValue(double value)
            : m_Type(ConfigurationValueType::Double)
        {
            SetImpl(value);
        }

        inline ConfigurationValueType GetType() const
        {
            return m_Type;
        }

        inline const Name& AsName() const
        {
            CheckType(ConfigurationValueType::String);
            return GetImpl<Name>();
        }

        inline StringSlice AsString() const
        {
            CheckType(ConfigurationValueType::String);
            return GetImpl<Name>();
        }

        inline int64_t AsInt() const
        {
            CheckType(ConfigurationValueType::Int);
            return GetImpl<int64_t>();
        }

        inline double AsDouble() const
        {
            CheckType(ConfigurationValueType::Double);
            return GetImpl<double>();
        }

        inline explicit operator bool() const
        {
            return m_Type != ConfigurationValueType::Null;
        }
    };


    class ConfigurationSection final : public festd::intrusive_list_node
    {
        // ConfigurationSection's destructor is never called, everything is allocated from a linear allocator.
        // All the memory is freed once upon process termination.
        std::pmr::memory_resource* m_pAllocator;
        festd::intrusive_list<ConfigurationSection> m_Children;
        Name m_Key;
        ConfigurationValue m_Value;

        inline ConfigurationSection* FindChild(StringSlice key)
        {
            Name keyName;
            if (!Name::TryGetExisting({ key.Data(), key.Size() }, keyName))
                return nullptr;

            for (ConfigurationSection& child : m_Children)
            {
                if (child.m_Key == keyName)
                    return &child;
            }

            return nullptr;
        }

        inline const ConfigurationSection* FindChild(StringSlice key) const
        {
            Name keyName;
            if (!Name::TryGetExisting({ key.Data(), key.Size() }, keyName))
                return nullptr;

            for (const ConfigurationSection& child : m_Children)
            {
                if (child.m_Key == keyName)
                    return &child;
            }

            return nullptr;
        }

    public:
        inline ConfigurationSection()
            : m_pAllocator(Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear))
        {
        }

        inline ConfigurationSection(std::pmr::memory_resource* pAllocator)
            : m_pAllocator(pAllocator)
        {
        }

        inline Name GetKey() const
        {
            return m_Key;
        }

        inline ConfigurationValue GetValue() const
        {
            return m_Value;
        }

        inline ConfigurationValue Get(StringSlice path) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return {};

            return pSection->m_Value;
        }

        inline Env::Name GetName(StringSlice path, Env::Name defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_Value.AsName();
        }

        inline StringSlice GetString(StringSlice path, StringSlice defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_Value.AsString();
        }

        inline int64_t GetInt(StringSlice path, int64_t defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_Value.AsInt();
        }

        inline double GetDouble(StringSlice path, double defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->m_Value.AsDouble();
        }

        inline void Set(StringSlice path, ConfigurationValue value)
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
                    pSection = Memory::New<ConfigurationSection>(m_pAllocator, m_pAllocator);
                    m_Children.push_back(*pSection);
                    pSection->m_Key = Name{ token.Data(), static_cast<uint32_t>(token.Size()) };
                }

                if (end == path.end())
                    break;

                begin = end;
                end = path.FindFirstOf(++end, '/');
            }

            pSection->m_Value = value;
        }

        inline const ConfigurationSection* GetSection(StringSlice path) const
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
    };


    class Configuration final : public Memory::RefCountedObjectBase
    {
        festd::intrusive_list<ConfigurationSection> m_RootSections;
        mutable SpinLock m_Lock;

    public:
        FE_RTTI_Class(Configuration, "FE804C2B-6671-47E7-8A96-4CD59680CCE4");

        inline void AddProvider(ConfigurationSection& providerSection)
        {
            std::lock_guard lk{ m_Lock };
            m_RootSections.push_back(providerSection);
        }

        inline const ConfigurationSection* GetSection(StringSlice path) const
        {
            std::lock_guard lk{ m_Lock };
            for (const ConfigurationSection& section : m_RootSections)
            {
                if (const ConfigurationSection* pSection = section.GetSection(path))
                    return pSection;
            }

            return nullptr;
        }

        inline ConfigurationValue Get(StringSlice path) const
        {
            std::lock_guard lk{ m_Lock };
            for (const ConfigurationSection& section : m_RootSections)
            {
                if (const ConfigurationValue value = section.Get(path))
                    return value;
            }

            return {};
        }

        inline Env::Name GetName(StringSlice path, Env::Name defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsName();
        }

        inline StringSlice GetString(StringSlice path, StringSlice defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsString();
        }

        inline int64_t GetInt(StringSlice path, int64_t defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsInt();
        }

        inline double GetDouble(StringSlice path, double defaultValue) const
        {
            const ConfigurationSection* pSection = GetSection(path);
            if (pSection == nullptr)
                return defaultValue;

            return pSection->GetValue().AsDouble();
        }
    };
} // namespace FE::Env
