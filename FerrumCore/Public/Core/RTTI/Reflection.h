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
        //! The reflected fields do not hold the whole value, e.g. bitfields, anonymous unions or a manual serializer's state.
        //! Field-wise operations such as copies treat the value as a whole.
        kHasUnreflectedFields = 1 << 4,
        //! Synthesized from a C++ type that has no generated reflection, e.g. a container instance.
        kSynthesized = 1 << 5,
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
        kTransient = 1 << 6,

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


        template<size_t TSize, class T>
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


    struct Type;


    struct FieldInfo final
    {
        festd::ascii_view m_name;
        //! Never null; C arrays, containers and other unreflected types are synthesized.
        const Type* m_type = nullptr;
        festd::span<const Attribute> m_attributes;
        uint32_t m_offset = 0;
        uint32_t m_size = 0;
        FieldFlags m_flags = FieldFlags::kNone;

        template<class T>
        [[nodiscard]] const T* TryGetAttribute() const
        {
            return Rtti::TryGetAttribute<T>(m_attributes);
        }


        [[nodiscard]] void* GetAddress(void* instance) const
        {
            return static_cast<std::byte*>(instance) + m_offset;
        }


        [[nodiscard]] const void* GetAddress(const void* instance) const
        {
            return static_cast<const std::byte*>(instance) + m_offset;
        }


        template<class TValue>
        const TValue& Get(const void* instance) const
        {
            return *static_cast<const TValue*>(GetAddress(instance));
        }


        template<class TValue>
        void Set(void* instance, const TValue& value) const
        {
            *static_cast<TValue*>(GetAddress(instance)) = value;
        }
    };


    //! How generic traversal, copying and editing see the values of a type.
    enum class TypeShape : uint8_t
    {
        //! No structure is exposed; the value can only be copied as a whole.
        kOpaque,
        //! Reflected fields and bases.
        kStruct,
        kScalar,
        kEnum,
        kString,
        //! Fixed number of contiguous elements, e.g. a C array.
        kFixedArray,
        //! Resizable list of elements.
        kSequence,
        //! One of several alternative types.
        kVariant,
        //! Value whose type is known only at runtime, e.g. Rc<T> or Any.
        kDynamic,
        //! UUID-valued identity; type attributes say what it refers to.
        kHandle,
    };


    enum class ScalarKind : uint8_t
    {
        kBool,
        kSigned,
        kUnsigned,
        kFloat,
    };


    struct ScalarShape final
    {
        static constexpr TypeShape kShape = TypeShape::kScalar;

        ScalarKind m_kind = ScalarKind::kSigned;
    };


    struct EnumShape final
    {
        static constexpr TypeShape kShape = TypeShape::kEnum;

        int64_t (*m_get)(const void* value) = nullptr;
        void (*m_set)(void* value, int64_t underlying) = nullptr;
    };


    struct StringShape final
    {
        static constexpr TypeShape kShape = TypeShape::kString;

        std::string_view (*m_get)(const void* value) = nullptr;
        //! Null for read-only strings such as views.
        void (*m_set)(void* value, std::string_view text) = nullptr;
    };


    struct FixedArrayShape final
    {
        static constexpr TypeShape kShape = TypeShape::kFixedArray;

        const Type* m_element = nullptr;
        uint32_t m_count = 0;
        uint32_t m_stride = 0;
    };


    struct SequenceShape final
    {
        static constexpr TypeShape kShape = TypeShape::kSequence;

        const Type* m_element = nullptr;
        uint32_t (*m_size)(const void* value) = nullptr;
        void* (*m_at)(void* value, uint32_t index) = nullptr;
        //! @brief Insert a default-constructed element; fails when the container is full.
        bool (*m_insert)(void* value, uint32_t index) = nullptr;
        void (*m_erase)(void* value, uint32_t index) = nullptr;
        //! @brief Resize with default-constructed elements; fails when the container cannot hold them.
        bool (*m_resize)(void* value, uint32_t size) = nullptr;
    };


    struct VariantShape final
    {
        static constexpr TypeShape kShape = TypeShape::kVariant;

        festd::span<const Type* const> m_alternatives;
        uint32_t (*m_index)(const void* value) = nullptr;
        void* (*m_get)(void* value) = nullptr;
        //! @brief Replace the value with a default-constructed alternative.
        void (*m_emplace)(void* value, uint32_t index) = nullptr;
    };


    struct DynamicShape final
    {
        static constexpr TypeShape kShape = TypeShape::kDynamic;

        //! Common base of the held values; null when any registered type can be held.
        const Type* m_base = nullptr;
        //! Most derived type of the held value; null when empty.
        const Type* (*m_type)(const void* value) = nullptr;
        void* (*m_value)(void* value) = nullptr;
        //! @brief Replace the held value with a default-constructed instance of type, or clear it when type is null.
        bool (*m_reset)(void* value, const Type* type) = nullptr;
    };


    struct HandleShape final
    {
        static constexpr TypeShape kShape = TypeShape::kHandle;

        Uuid (*m_get)(const void* value) = nullptr;
        void (*m_set)(void* value, Uuid uuid) = nullptr;
    };


    struct Any;


    struct Type final : public festd::intrusive_list_node
    {
        using DefaultConstructor = void (*)(void*);
        using CopyConstructor = void (*)(void*, const void*);
        using CopyAssignment = void (*)(void*, const void*);
        using MoveConstructor = void (*)(void*, void*);
        using Destructor = void (*)(void*);
        //! @brief Cast a complete polymorphic instance to a reflected base, adjusting its address when needed.
        using Cast = void* (*)(void*, TypeID);
        using Serialize = Serialization::ResultCode (*)(Serialization::SerializationContext&, const void*);
        using Deserialize = Serialization::ResultCode (*)(Serialization::DeserializationContext&, void*);

        TypeID m_id = TypeID::kNull;
        festd::ascii_view m_name;
        festd::ascii_view m_qualifiedName;
        DefaultConstructor m_defaultConstructor = nullptr;
        CopyConstructor m_copyConstructor = nullptr;
        CopyAssignment m_copyAssignment = nullptr;
        MoveConstructor m_moveConstructor = nullptr;
        Destructor m_destructor = nullptr;
        //! @brief Type-erased RTTI cast; null for types without polymorphic RTTI.
        Cast m_cast = nullptr;
        Serialize m_serialize = nullptr;
        Deserialize m_deserialize = nullptr;
        bool (*m_validate)(const void*) = nullptr;
        bool (*m_configure)(void*, const Any&) = nullptr;
        //! Apply settings at a world safe point after initialization; absent for startup-only configuration.
        bool (*m_applyConfiguration)(void*, const Any&) = nullptr;
        Any (*m_captureConfiguration)(const void*) = nullptr;
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
        TypeShape m_shape = TypeShape::kOpaque;
        //! Points to the shape structure matching m_shape; null for kOpaque and kStruct.
        const void* m_shapeInfo = nullptr;

        template<class T>
        [[nodiscard]] const T* TryGetAttribute() const
        {
            return Rtti::TryGetAttribute<T>(m_attributes);
        }


        template<class TShape>
        [[nodiscard]] const TShape* TryGetShape() const
        {
            return m_shape == TShape::kShape ? static_cast<const TShape*>(m_shapeInfo) : nullptr;
        }


        template<class TShape>
        void SetShape(const TShape& shape)
        {
            m_shape = TShape::kShape;
            m_shapeInfo = &shape;
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

        namespace Internal
        {
            //! @brief Register a type synthesized on first use; runs while reflecting the types that contain it.
            void RegisterSynthesizedType(Type& type);
        } // namespace Internal
    } // namespace TypeRegistry


    struct TypeRegistrar final
    {
        using RegisterFunc = void (*)(ReflectionContext&);

        explicit TypeRegistrar(RegisterFunc func) noexcept;
    };
} // namespace FE::Rtti
