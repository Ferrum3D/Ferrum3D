{% if type.is_builtin %}
namespace FE::RTTI
{
    void Internal::ExternalTypeReflector<{{ type.name }}>::Reflect(ReflectionContext& context)
    {
        static Type typeInstance;

        static constexpr alignas(16) uint8_t kTypeIDBytes[sizeof(TypeID)] = {
            {% for b in type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ type.name }}
        };

        context.ReflectBuiltinType<{{ type.name }}>(typeInstance, TypeID::LoadAligned(kTypeIDBytes), "{{ type.name }}");
    }

    static TypeRegistrar GTypeRegistrar_{{ type.id.bytes.hex() }}(&Internal::ExternalTypeReflector<{{ type.name }}>::Reflect);
}
{% elif type.is_external %}
namespace FE::RTTI
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

        static constexpr festd::array<RTTI::Attribute, {{ type.attributes|length }}> kAttributes = {
            {%- for attribute in type.attributes %}
            RTTI::Attribute{ {{ attribute[0], attribute[1] }} },
            {%- endfor %}
        };
        {% for field in type.fields %}
        static constexpr festd::array<RTTI::Attribute, {{ field.attributes|length }}> kAttributes_{{ field.name }} = {
            {%- for attribute in field.attributes %}
            RTTI::Attribute{ {{ attribute[0], attribute[1] }} },
            {% endfor %}
        };
        {% endfor %}
        static const festd::array<RTTI::FieldInfo, {{ type.fields|length }}> kFields = {
            {%- for field in type.fields %}
            RTTI::ReflectionContext::CreateFieldInfo("{{ field.name }}",
                                                     &{{ type.qualified_name }}::{{ field.name }},
                                                     kAttributes_{{ field.name }},
                                                     {{ field.flags }}),
            {%- endfor %}
        };

        context.ReflectClass<{{ type.qualified_name }}>(typeInstance, UUID::LoadAligned(kTypeIDBytes), "{{ type.qualified_name }}", {}, kAttributes, kFields);
    }

    static RTTI::TypeRegistrar GTypeRegistrar_{{ type.id.bytes.hex() }}(&Internal::ExternalTypeReflector<{{ type.qualified_name }}>::Reflect);
}
{% else %}
namespace {{ type.namespace }}
{
    const RTTI::TypeID {{ type.name }}::TypeID = RTTI::TypeID{ "{{ type.id }}" };

    namespace
    {
        FE_FORCE_INLINE void* FE_VECTORCALL RTTI_TryCastImpl_{{ type.id.bytes.hex() }}({{ type.name }}* thisPtr, const RTTI::TypeID typeID)
        {
            static constexpr alignas(16) uint8_t kBaseClassTypeIDs[{{ type.bases|length + 1 }} * sizeof(RTTI::TypeID)] = {
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
            id = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kBaseClassTypeIDs + {{ loop.index }} * sizeof(RTTI::TypeID)));
            mask = _mm_cmpeq_epi8(id, typeID.m_simdVector);
            if (_mm_movemask_epi8(mask) == 0xffff)
                return static_cast<{{ base.qualified_name }}*>(thisPtr);

            {%- endfor %}

            return nullptr;
        }
    }

    void* FE_VECTORCALL {{ type.name }}::RTTI_TryCast(const RTTI::TypeID typeID)
    {
        return RTTI_TryCastImpl_{{ type.id.bytes.hex() }}(this, typeID);
    }


    const void* FE_VECTORCALL {{ type.name }}::RTTI_TryCast(const RTTI::TypeID typeID) const
    {
        return RTTI_TryCastImpl_{{ type.id.bytes.hex() }}(const_cast<{{ type.name }}*>(this), typeID);
    }


    void {{ type.name }}::Reflect(RTTI::ReflectionContext& context)
    {
        static RTTI::Type typeInstance;

        static constexpr alignas(16) uint8_t kTypeIDBytes[sizeof(RTTI::TypeID)] = {
            {% for b in type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ type.qualified_name }}
        };

        static constexpr alignas(16) festd::array<uint8_t, {{ type.bases|length }} * sizeof(RTTI::TypeID)> kBaseClassTypeIDs = {
            {%- for base in type.bases %}
            {% for b in base.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ base.qualified_name }}
            {%- endfor %}
        };

        static constexpr festd::array<RTTI::Attribute, {{ type.attributes|length }}> kAttributes = {
            {%- for attribute in type.attributes %}
            RTTI::Attribute{ {{ attribute[0], attribute[1] }} },
            {%- endfor %}
        };
        {% for field in type.fields %}
        static constexpr festd::array<RTTI::Attribute, {{ field.attributes|length }}> kAttributes_{{ field.name }} = {
            {%- for attribute in field.attributes %}
            RTTI::Attribute{ {{ attribute[0], attribute[1] }} },
            {% endfor %}
        };
        {% endfor %}
        static const festd::array<RTTI::FieldInfo, {{ type.fields|length }}> kFields = {
            {%- for field in type.fields %}
            RTTI::ReflectionContext::CreateFieldInfo("{{ field.name }}",
                                                     &{{ type.name }}::{{ field.name }},
                                                     kAttributes_{{ field.name }},
                                                     {{ field.flags }}),
            {%- endfor %}
        };

        context.ReflectClass<{{ type.name }}>(typeInstance, UUID::LoadAligned(kTypeIDBytes), "{{ type.qualified_name }}", kBaseClassTypeIDs, kAttributes, kFields);
    }

    static RTTI::TypeRegistrar GTypeRegistrar_{{ type.id.bytes.hex() }}(&{{ type.name }}::Reflect);
}
{% endif %}
