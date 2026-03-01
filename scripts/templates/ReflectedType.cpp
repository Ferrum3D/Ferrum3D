{% if type.is_external %}
namespace FE::Rtti
{
    namespace
    {
        Type& GetMutableType_{{ type.id.bytes.hex() }}()
        {
            static Type typeInstance;
            return typeInstance;
        }
    }

    template<>
    const Type& GetType<{{ type.qualified_name }}>()
    {
        return GetMutableType_{{ type.id.bytes.hex() }}();
    }

    template<>
    TypeID GetTypeID<{{ type.qualified_name }}>()
    {
        static constexpr alignas(16) uint8_t kTypeIDBytes[sizeof(TypeID)] = {
            {% for b in type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ type.qualified_name }}
        };

        return TypeID::LoadAligned(kTypeIDBytes);
    }

    void Internal::ExternalTypeReflector<{{ type.qualified_name }}>::Reflect(ReflectionContext& context)
    {
        Type& typeInstance = GetMutableType_{{ type.id.bytes.hex() }}();

        static constexpr alignas(16) uint8_t kTypeIDBytes[sizeof(TypeID)] = {
            {% for b in type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ type.qualified_name }}
        };
{% if type.is_builtin %}
        context.ReflectBuiltinType<{{ type.qualified_name }}>(typeInstance, TypeID::LoadAligned(kTypeIDBytes), "{{ type.qualified_name }}");
{%- elif type.is_enum %}
        static constexpr alignas(16) uint8_t kUnderlyingTypeIDBytes[sizeof(TypeID)] = {
            {% for b in type.bases[0].id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ type.bases[0].qualified_name }}
        };

        static constexpr festd::array<Rtti::Attribute, {{ type.attributes|length }}> kAttributes = {
            {%- for attribute in type.attributes %}
            Rtti::Attribute{ {{ attribute[0], attribute[1] }} },
            {%- endfor %}
        };

        static constexpr festd::array<festd::ascii_view, {{ type.fields|length }}> kEnumNames = {
            {%- for field in type.fields %}
            "{{ field.name }}",
            {%- endfor %}
        };

        static constexpr festd::array<festd::ascii_view, {{ type.fields|length }}> kEnumDisplayNames = {
            {%- for field in type.fields %}
            "{{ field.display_name }}",
            {%- endfor %}
        };

        static constexpr festd::array<int64_t, {{ type.fields|length }}> kEnumValues = {
            {%- for field in type.fields %}
            static_cast<{{ type.bases[0].qualified_name }}>({{ field.enum_value }}),
            {%- endfor %}
        };

        context.ReflectEnum<{{ type.qualified_name }}>(typeInstance, TypeID::LoadAligned(kTypeIDBytes), kUnderlyingTypeIDBytes,
            "{{ type.qualified_name }}", kAttributes, kEnumNames, kEnumDisplayNames, kEnumValues);
{%- else %}
        {%- if type.fields|length > 0 %}
        static constexpr alignas(16) uint8_t kFieldTypeIDs[{{ type.fields|length }} * sizeof(TypeID)] = {
            {%- for field in type.fields %}
            {%- if field.type %}
            {% for b in field.type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ field.type.qualified_name }} {{ field.name }}
            {%- else %}
            {{ '0x00, ' * 16 }} // <unknown> {{ field.name }}
            {%- endif %}
            {%- endfor %}
        };

        {%- endif %}
        static constexpr festd::array<Rtti::Attribute, {{ type.attributes|length }}> kAttributes = {
            {%- for attribute in type.attributes %}
            Rtti::Attribute{ {{ attribute[0], attribute[1] }} },
            {%- endfor %}
        };
        {% for field in type.fields %}
        static constexpr festd::array<Rtti::Attribute, {{ field.attributes|length }}> kAttributes_{{ field.name }} = {
            {%- for attribute in field.attributes %}
            Rtti::Attribute{ {{ attribute[0], attribute[1] }} },
            {% endfor %}
        };
        {% endfor %}
        static const festd::array<Rtti::FieldInfo, {{ type.fields|length }}> kFields = {
            {%- for field in type.fields %}
            Rtti::ReflectionContext::CreateFieldInfo<{{ field.array_size }}>("{{ field.name }}",
                                                     TypeID::LoadAligned(kFieldTypeIDs + {{ loop.index0 }} * sizeof(TypeID)),
                                                     &{{ type.qualified_name }}::{{ field.name }},
                                                     kAttributes_{{ field.name }},
                                                     {{ field.flags }}),
            {%- endfor %}
        };

        context.ReflectClass<{{ type.qualified_name }}>(typeInstance, Rtti::TypeID::LoadAligned(kTypeIDBytes), "{{ type.qualified_name }}", {}, kAttributes, kFields);
{% endif %}
    }

    static TypeRegistrar GTypeRegistrar_{{ type.id.bytes.hex() }}(&Internal::ExternalTypeReflector<{{ type.qualified_name }}>::Reflect);
}
{% else %}
namespace {{ type.namespace }}
{
    const Rtti::TypeID {{ type.name }}::TypeID = Rtti::TypeID{ "{{ type.id }}" };

    namespace
    {
        FE_FORCE_INLINE void* FE_VECTORCALL RTTI_TryCastImpl_{{ type.id.bytes.hex() }}({{ type.name }}* thisPtr, const Rtti::TypeID typeID)
        {
            static constexpr alignas(16) uint8_t kBaseClassTypeIDs[{{ type.bases|length + 1 }} * sizeof(Rtti::TypeID)] = {
                {% for b in type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ type.qualified_name }} (this type)
                {%- for base in type.bases %}
                {% for b in base.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ base.qualified_name }}
                {%- endfor %}
            };

            __m128i id = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kBaseClassTypeIDs));
            __m128i mask = _mm_cmpeq_epi8(id, typeID.m_simdVector);
            if (_mm_movemask_epi8(mask) == 0xffff)
                return thisPtr;
            {%- for base in type.bases %}
            id = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kBaseClassTypeIDs + {{ loop.index }} * sizeof(Rtti::TypeID)));
            mask = _mm_cmpeq_epi8(id, typeID.m_simdVector);
            if (_mm_movemask_epi8(mask) == 0xffff)
                return static_cast<{{ base.qualified_name }}*>(thisPtr);

            {%- endfor %}

            return nullptr;
        }

        Rtti::Type& RTTI_GetMutableType_{{ type.id.bytes.hex() }}()
        {
            static Rtti::Type typeInstance;
            return typeInstance;
        }
    }

    const Rtti::Type& {{ type.name }}::RTTI_GetType()
    {
        return RTTI_GetMutableType_{{ type.id.bytes.hex() }}();
    }

    void* FE_VECTORCALL {{ type.name }}::RTTI_TryCast(const Rtti::TypeID typeID)
    {
        return RTTI_TryCastImpl_{{ type.id.bytes.hex() }}(this, typeID);
    }

    const void* FE_VECTORCALL {{ type.name }}::RTTI_TryCast(const Rtti::TypeID typeID) const
    {
        return RTTI_TryCastImpl_{{ type.id.bytes.hex() }}(const_cast<{{ type.name }}*>(this), typeID);
    }

    void {{ type.name }}::Reflect(Rtti::ReflectionContext& context)
    {
        Rtti::Type& typeInstance = RTTI_GetMutableType_{{ type.id.bytes.hex() }}();

        static constexpr alignas(16) uint8_t kTypeIDBytes[sizeof(Rtti::TypeID)] = {
            {% for b in type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ type.qualified_name }}
        };

        static constexpr alignas(16) festd::array<uint8_t, {{ type.bases|length }} * sizeof(Rtti::TypeID)> kBaseClassTypeIDs = {
            {%- for base in type.bases %}
            {% for b in base.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ base.qualified_name }}
            {%- endfor %}
        };

        static constexpr festd::array<Rtti::Attribute, {{ type.attributes|length }}> kAttributes = {
            {%- for attribute in type.attributes %}
            Rtti::Attribute{ {{ attribute[0], attribute[1] }} },
            {%- endfor %}
        };
        {% for field in type.fields %}
        static constexpr festd::array<Rtti::Attribute, {{ field.attributes|length }}> kAttributes_{{ field.name }} = {
            {%- for attribute in field.attributes %}
            Rtti::Attribute{ {{ attribute[0], attribute[1] }} },
            {% endfor %}
        };
        {% endfor %}
        static const festd::array<Rtti::FieldInfo, {{ type.fields|length }}> kFields = {
            {%- for field in type.fields %}
            Rtti::ReflectionContext::CreateFieldInfo<{{ field.array_size }}>("{{ field.name }}",
                                                     &{{ type.name }}::{{ field.name }},
                                                     kAttributes_{{ field.name }},
                                                     {{ field.flags }}),
            {%- endfor %}
        };

        context.ReflectClass<{{ type.name }}>(typeInstance, Rtti::TypeID::LoadAligned(kTypeIDBytes), "{{ type.qualified_name }}", kBaseClassTypeIDs, kAttributes, kFields);
    }

    static Rtti::TypeRegistrar GTypeRegistrar_{{ type.id.bytes.hex() }}(&{{ type.name }}::Reflect);
}
{% endif %}
