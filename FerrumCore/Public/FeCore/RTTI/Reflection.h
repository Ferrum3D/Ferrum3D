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
        kTrivial = 1 << 2,
        kStandardLayout = 1 << 3,
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
        uint32_t m_arraySize = 0;
        FieldFlags m_flags = FieldFlags::kNone;

        template<class TValue>
        const TValue& Get(const void* instance, const uint32_t arrayIndex = 0) const
        {
            FE_AssertDebug(arrayIndex < m_arraySize);

            const uintptr_t address = reinterpret_cast<uintptr_t>(instance) + m_offset;
            const TValue* ptr = reinterpret_cast<const TValue*>(address);
            return ptr[arrayIndex];
        }

        template<class TValue>
        void Set(void* instance, const TValue& value, const uint32_t arrayIndex = 0) const
        {
            FE_AssertDebug(arrayIndex < m_arraySize);

            const uintptr_t address = reinterpret_cast<uintptr_t>(instance) + m_offset;
            TValue* ptr = reinterpret_cast<TValue*>(address);
            ptr[arrayIndex] = value;
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
        festd::span<const festd::ascii_view> m_enumDisplayNames;
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
} // namespace FE::RTTI
