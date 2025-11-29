#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <festd/intrusive_list.h>

namespace FE::RTTI
{
    enum class TypeFlags : uint32_t
    {
        kNone = 0,
        kClass = 1 << 0,
        kEnum = 1 << 1,
    };

    FE_ENUM_OPERATORS(TypeFlags);


    enum class FieldFlags : uint32_t
    {
        kNone = 0,
        kStatic = 1 << 0,
        kInstance = 1 << 1,
        kPrivate = 1 << 2,
        kProtected = 1 << 3,
        kPublic = 1 << 4,
        kPointer = 1 << 5,

        kAll = Constants::kMaxU32,
    };

    FE_ENUM_OPERATORS(FieldFlags);


    struct Attribute final
    {
        festd::ascii_view m_key;
        festd::ascii_view m_value;
    };


    struct FieldInfo final
    {
        festd::ascii_view m_name;
        TypeID m_type;
        festd::span<const Attribute> m_attributes;
        uint32_t m_offset = 0;
        uint32_t m_size = 0;
        FieldFlags m_flags = FieldFlags::kNone;

        template<class T, class TValue>
        void Set(T* instance, const TValue& value) const
        {
            TValue* prev = reinterpret_cast<TValue*>(reinterpret_cast<uint8_t*>(instance) + m_offset);
            *prev = value;
        }
    };


    struct Type final : public festd::intrusive_list_node
    {
        TypeID m_id = TypeID::kNull;
        festd::ascii_view m_name;
        festd::ascii_view m_qualifiedName;
        festd::span<const TypeID> m_baseTypes;
        festd::span<const Attribute> m_attributes;
        festd::span<const FieldInfo> m_fields;
        festd::span<const festd::ascii_view> m_enumNames;
        festd::span<const int64_t> m_enumValues;
        uint32_t m_size = 0;
        uint32_t m_alignment = 0;
        TypeFlags m_flags = TypeFlags::kNone;
    };


    struct TypeList final
    {
        explicit TypeList(const festd::intrusive_list<Type>& list)
            : m_list(&list)
        {
        }

        [[nodiscard]] festd::intrusive_list<Type>::const_iterator begin() const
        {
            return m_list->begin();
        }

        [[nodiscard]] festd::intrusive_list<Type>::const_iterator end() const
        {
            return m_list->end();
        }

    private:
        const festd::intrusive_list<Type>* m_list;
    };


    namespace TypeRegistry
    {
        const Type* FindType(UUID id);
        const Type* FindType(festd::ascii_view qualifiedName);

        TypeList GetTypes();
    } // namespace TypeRegistry


    struct TypeRegistrar final
    {
        using RegisterFunc = void (*)(ReflectionContext&);

        explicit TypeRegistrar(RegisterFunc func);
    };


    FE_RTTI_Reflect(uint8_t, "80E074D8-C4C0-4190-B716-701DBA47F9F7");
    FE_RTTI_Reflect(uint16_t, "DA7E1828-9EC2-408B-A93B-50F254939992");
    FE_RTTI_Reflect(uint32_t, "334F0750-1B4E-4F4C-AC6F-985382D4BD11");
    FE_RTTI_Reflect(uint64_t, "76E0D616-A646-422A-B506-07ADBE41756B");
    FE_RTTI_Reflect(int8_t, "B9AE58D6-7AA1-4179-9F4B-03A57872BBD0");
    FE_RTTI_Reflect(int16_t, "99CD9FE5-B954-41DD-BF51-1F2DB4E6D433");
    FE_RTTI_Reflect(int32_t, "174196BD-8BFE-4049-B72E-8A07AD372659");
    FE_RTTI_Reflect(int64_t, "8B225D72-D811-437E-BC70-955533A9B84E");
    FE_RTTI_Reflect(float, "66D8930B-3458-4847-B6F1-002C70DC1ED2");
    FE_RTTI_Reflect(double, "37BB8848-1962-4773-93DF-9E7DCD06668E");
    FE_RTTI_Reflect(bool, "DD3BA9BB-E7D2-4217-A797-F7C81EF351A2");

    FE_RTTI_Reflect(UUID, "338EBFD9-5113-4193-B0F0-C89A4AC7D064");

} // namespace FE::RTTI
