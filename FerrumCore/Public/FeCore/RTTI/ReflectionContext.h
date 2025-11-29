#pragma once
#include <FeCore/RTTI/Reflection.h>

namespace FE::RTTI
{
    struct ReflectionContext
    {
        virtual ~ReflectionContext() = default;

        ReflectionContext(const ReflectionContext&) = delete;
        ReflectionContext(ReflectionContext&&) = delete;
        ReflectionContext& operator=(const ReflectionContext&) = delete;
        ReflectionContext& operator=(ReflectionContext&&) = delete;

        template<class T>
        void ReflectClass(Type& type, const TypeID id, const festd::ascii_view qualifiedName,
                          const festd::span<const uint8_t> baseTypes, const festd::span<const Attribute> attributes,
                          const festd::span<const FieldInfo> fields)
        {
            type.m_id = id;
            type.m_name = GetShortName(qualifiedName);
            type.m_qualifiedName = qualifiedName;
            type.m_baseTypes = festd::span(reinterpret_cast<const TypeID*>(baseTypes.data()), baseTypes.size() / sizeof(TypeID));
            type.m_attributes = attributes;
            type.m_fields = fields;
            type.m_size = sizeof(T);
            type.m_alignment = alignof(T);

            RegisterType(type);
        }

        template<class T>
        void ReflectEnum(Type& type, const TypeID id, const festd::ascii_view qualifiedName,
                         const festd::span<const Attribute> attributes, const festd::span<const festd::ascii_view> names,
                         const festd::span<const int64_t> values)
        {
            type.m_id = id;
            type.m_name = GetShortName(qualifiedName);
            type.m_qualifiedName = qualifiedName;
            type.m_baseTypes = festd::span<const UUID>(&GetTypeID<std::underlying_type_t<T>>(), 1);
            type.m_attributes = attributes;
            type.m_enumNames = names;
            type.m_enumValues = values;
            type.m_size = sizeof(T);
            type.m_alignment = alignof(T);

            RegisterType(type);
        }

        template<class T>
        void ReflectBuiltinType(Type& type, const TypeID id, const festd::ascii_view name)
        {
            type.m_id = id;
            type.m_name = name;
            type.m_qualifiedName = name;
            type.m_size = sizeof(T);
            type.m_alignment = alignof(T);

            RegisterType(type);
        }

        template<class TClass, class TField>
        static FieldInfo CreateFieldInfo(const festd::ascii_view name, TField TClass::* field,
                                         const festd::span<const Attribute> attributes, const FieldFlags flags)
        {
            const auto fieldOffset = reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<TClass const volatile*>(0)->*field));

            FieldInfo fieldInfo;
            fieldInfo.m_name = name;
            fieldInfo.m_type = GetTypeID<TField>();
            fieldInfo.m_attributes = attributes;
            fieldInfo.m_offset = static_cast<uint32_t>(fieldOffset);
            fieldInfo.m_size = sizeof(TField);
            fieldInfo.m_flags = flags;

            if (std::is_pointer_v<TField>)
                fieldInfo.m_flags |= FieldFlags::kPointer;

            return fieldInfo;
        }

    protected:
        ReflectionContext() = default;

        virtual void RegisterType(Type& type) = 0;

    private:
        static festd::ascii_view GetShortName(festd::ascii_view qualifiedName)
        {
            if (const auto colonIndex = qualifiedName.find_last_of(':', qualifiedName.find_first_of('<'));
                colonIndex != festd::ascii_view::npos)
            {
                qualifiedName.remove_prefix(colonIndex + 1);
            }

            return qualifiedName;
        }
    };
} // namespace FE::RTTI
