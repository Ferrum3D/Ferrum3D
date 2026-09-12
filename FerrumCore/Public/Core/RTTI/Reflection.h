#pragma once
#include <Core/Base/Base.h>
#include <festd/intrusive_list.h>

namespace FE::Serialization
{
    enum class ResultCode : int32_t;
    struct SerializationContext;
    struct DeserializationContext;
} // namespace FE::Serialization

namespace FE::Rtti
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
        uint64_t m_typeNameHash = 0;
        const void* m_value = nullptr;
    };


    namespace Internal
    {
        template<class T>
        struct MetaAttributeCount
        {
            static_assert(std::derived_from<T, MetaAttributeBase>);
            static constexpr uint32_t kValue = 1;
        };

        template<class TLeft, class TRight>
        struct MetaAttributeCount<MetaAttributeList<TLeft, TRight>>
        {
            static constexpr uint32_t kValue = MetaAttributeCount<TLeft>::kValue + MetaAttributeCount<TRight>::kValue;
        };


        template<uint32_t TSize, class T>
        constexpr void DescribeMetaAttribute(festd::array<Attribute, TSize>& result, uint32_t& index, const T& value)
        {
            if constexpr (kIsMetaAttributeList<T>)
            {
                DescribeMetaAttribute(result, index, value.m_left);
                DescribeMetaAttribute(result, index, value.m_right);
            }
            else
            {
                static_assert(std::derived_from<T, MetaAttributeBase>);
                result[index++] = Attribute{ .m_typeNameHash = TypeNameHash<T>, .m_value = &value };
            }
        }
    } // namespace Internal


    template<class T>
    [[nodiscard]] constexpr auto DescribeAttributes(const T& values)
    {
        constexpr uint32_t kAttributeCount = Internal::MetaAttributeCount<T>::kValue;
        festd::array<Attribute, kAttributeCount> result{};
        uint32_t index = 0;
        Internal::DescribeMetaAttribute(result, index, values);
        return result;
    }


    template<class T>
    [[nodiscard]] const T* TryGetAttribute(const festd::span<const Attribute> attributes)
    {
        static_assert(std::derived_from<T, MetaAttributeBase>);

        for (const Attribute& attribute : attributes)
        {
            if (attribute.m_typeNameHash == TypeNameHash<T>)
                return static_cast<const T*>(attribute.m_value);
        }

        return nullptr;
    }


    struct FieldInfo final
    {
        festd::ascii_view m_name;
        TypeID m_type;
        festd::span<const Attribute> m_attributes;
        uint32_t m_offset = 0;
        uint32_t m_size = 0;
        uint32_t m_arraySize = 0;
        FieldFlags m_flags = FieldFlags::kNone;

        template<class T>
        [[nodiscard]] const T* TryGetAttribute() const
        {
            return Rtti::TryGetAttribute<T>(m_attributes);
        }

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
        using DefaultConstructor = void (*)(void*);
        using Destructor = void (*)(void*);
        using Serialize = Serialization::ResultCode (*)(Serialization::SerializationContext&, const void*);
        using Deserialize = Serialization::ResultCode (*)(Serialization::DeserializationContext&, void*);

        TypeID m_id = TypeID::kNull;
        festd::ascii_view m_name;
        festd::ascii_view m_qualifiedName;
        DefaultConstructor m_defaultConstructor = nullptr;
        Destructor m_destructor = nullptr;
        Serialize m_serialize = nullptr;
        Deserialize m_deserialize = nullptr;
        festd::span<const TypeID> m_baseTypes;
        festd::span<const Attribute> m_attributes;
        festd::span<const FieldInfo> m_fields;
        festd::span<const festd::ascii_view> m_enumNames;
        festd::span<const festd::ascii_view> m_enumDisplayNames;
        festd::span<const int64_t> m_enumValues;
        uint32_t m_size = 0;
        uint32_t m_alignment = 0;
        uint32_t m_serializationVersion = 0;
        uint64_t m_serializationSchemaHash = 0;
        TypeFlags m_flags = TypeFlags::kNone;

        template<class T>
        [[nodiscard]] const T* TryGetAttribute() const
        {
            return Rtti::TryGetAttribute<T>(m_attributes);
        }
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
        const Type* FindType(Uuid id);
        const Type* FindType(festd::ascii_view qualifiedName);

        TypeList GetTypes();
    } // namespace TypeRegistry


    struct TypeRegistrar final
    {
        using RegisterFunc = void (*)(ReflectionContext&);

        explicit TypeRegistrar(RegisterFunc func) noexcept;
    };
} // namespace FE::Rtti
