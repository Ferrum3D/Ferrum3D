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
            {%- for attribute in type.display_attributes %}
            Rtti::Attribute{ .m_key = "{{ attribute[0] }}", .m_value = "{{ attribute[1] }}" },
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
        {%- if type.reflection_fields|length > 0 %}
        static constexpr alignas(16) uint8_t kFieldTypeIDs[{{ type.reflection_fields|length }} * sizeof(TypeID)] = {
            {%- for field in type.reflection_fields %}
            {%- if field.type %}
            {% for b in field.type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ field.type.qualified_name }} {{ field.name }}
            {%- else %}
            {{ '0x00, ' * 16 }} // <unknown> {{ field.name }}
            {%- endif %}
            {%- endfor %}
        };

        {%- endif %}
        static constexpr festd::array<Rtti::Attribute, {{ type.attributes|length }}> kAttributes = {
            {%- for attribute in type.display_attributes %}
            Rtti::Attribute{ .m_key = "{{ attribute[0] }}", .m_value = "{{ attribute[1] }}" },
            {%- endfor %}
        };
        {% for field in type.reflection_fields %}
        static constexpr festd::array<Rtti::Attribute, {{ field.attributes|length }}> kAttributes_{{ field.name }} = {
            {%- for attribute in field.display_attributes %}
            Rtti::Attribute{ .m_key = "{{ attribute[0] }}", .m_value = "{{ attribute[1] }}" },
            {% endfor %}
        };
        {% endfor %}
        static const festd::array<Rtti::FieldInfo, {{ type.reflection_fields|length }}> kFields = {
            {%- for field in type.reflection_fields %}
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
    const Rtti::TypeID {{ type.name }}::TypeID = Rtti::TypeID{
        {% for b in type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %}
    };

    namespace
    {
{% if not type.is_member -%}
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
{% endif -%}

        Rtti::Type& RTTI_GetMutableType_{{ type.id.bytes.hex() }}()
        {
            static Rtti::Type typeInstance;
            return typeInstance;
        }
{% if type.is_default_constructible %}
        void RTTI_DefaultConstruct_{{ type.id.bytes.hex() }}(void* storage)
        {
            ::new (storage) {{ type.name }}();
        }
{% endif -%}
    }

    const Rtti::Type& {{ type.name }}::RTTI_GetType()
    {
        return RTTI_GetMutableType_{{ type.id.bytes.hex() }}();
    }

{% if not type.is_member -%}
    void* FE_VECTORCALL {{ type.name }}::RTTI_TryCast(const Rtti::TypeID typeID)
    {
        return RTTI_TryCastImpl_{{ type.id.bytes.hex() }}(this, typeID);
    }

    const void* FE_VECTORCALL {{ type.name }}::RTTI_TryCast(const Rtti::TypeID typeID) const
    {
        return RTTI_TryCastImpl_{{ type.id.bytes.hex() }}(const_cast<{{ type.name }}*>(this), typeID);
    }
{% endif -%}

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

        {%- if type.reflection_fields|length > 0 %}
        static constexpr alignas(16) uint8_t kFieldTypeIDs[{{ type.reflection_fields|length }} * sizeof(Rtti::TypeID)] = {
            {%- for field in type.reflection_fields %}
            {%- if field.type %}
            {% for b in field.type.id.bytes %}{{ '0x%02x' % b }}, {% endfor %} // {{ field.type.qualified_name }} {{ field.name }}
            {%- else %}
            {{ '0x00, ' * 16 }} // <unknown> {{ field.name }}
            {%- endif %}
            {%- endfor %}
        };

        {%- endif %}
        static constexpr festd::array<Rtti::Attribute, {{ type.attributes|length }}> kAttributes = {
            {%- for attribute in type.display_attributes %}
            Rtti::Attribute{ .m_key = "{{ attribute[0] }}", .m_value = "{{ attribute[1] }}" },
            {%- endfor %}
        };
        {% for field in type.reflection_fields %}
        static constexpr festd::array<Rtti::Attribute, {{ field.attributes|length }}> kAttributes_{{ field.name }} = {
            {%- for attribute in field.display_attributes %}
            Rtti::Attribute{ .m_key = "{{ attribute[0] }}", .m_value = "{{ attribute[1] }}" },
            {% endfor %}
        };
        {% endfor %}
        static const festd::array<Rtti::FieldInfo, {{ type.reflection_fields|length }}> kFields = {
            {%- for field in type.reflection_fields %}
            Rtti::ReflectionContext::CreateFieldInfo<{{ field.array_size }}>("{{ field.name }}",
                                                     Rtti::TypeID::LoadAligned(kFieldTypeIDs + {{ loop.index0 }} * sizeof(TypeID)),
                                                     &{{ type.name }}::{{ field.name }},
                                                     kAttributes_{{ field.name }},
                                                     {{ field.flags }}),
            {%- endfor %}
        };

        context.ReflectClass<{{ type.name }}>(typeInstance, Rtti::TypeID::LoadAligned(kTypeIDBytes), "{{ type.qualified_name }}", kBaseClassTypeIDs, kAttributes, kFields
            {%- if type.is_default_constructible %}, &RTTI_DefaultConstruct_{{ type.id.bytes.hex() }} {% endif %}
        );
    }

    static Rtti::TypeRegistrar GTypeRegistrar_{{ type.id.bytes.hex() }}(&{{ type.name }}::Reflect);
{%- if type.is_serializable %}

    bool {{ type.name }}::RTTI_Serialize(FE::Serialization::SerializationContext& context) const
    {
        if (!context.BeginObject())
            return false;
        {%- for base in type.direct_bases %}
        context.Field("$base:{{ base.qualified_name }}", static_cast<const {{ base.qualified_name }}&>(*this));
        {%- endfor %}
        {%- for field in type.serialization_fields %}
        {%- if field.is_bitfield %}
        const auto value_{{ field.name }} = {{ field.name }};
        context.Field("{{ field.name }}", value_{{ field.name }});
        {%- else %}
        context.Field("{{ field.name }}", {{ field.name }});
        {%- endif %}
        {%- endfor %}
        context.EndObject();
        return context.IsValid();
    }

    bool {{ type.name }}::RTTI_Deserialize(FE::Serialization::SerializationContext& context)
    {
        if (!context.BeginObject())
            return false;
        {%- for base in type.direct_bases %}
        context.Field("$base:{{ base.qualified_name }}", static_cast<{{ base.qualified_name }}&>(*this));
        {%- endfor %}
        {%- for field in type.serialization_fields %}
        {%- if field.is_bitfield %}
        auto value_{{ field.name }} = {{ field.name }};
        context.Field("{{ field.name }}", value_{{ field.name }});
        {{ field.name }} = value_{{ field.name }};
        {%- else %}
        context.Field("{{ field.name }}", {{ field.name }});
        {%- endif %}
        {%- endfor %}
        context.EndObject();
        return context.IsValid();
    }

    uint64_t {{ type.name }}::RTTI_GetSerializationSchemaHash()
    {
        uint64_t result = FE::Serialization::Internal::kSchemaSeed;
        {%- for base in type.direct_bases %}
        result = FE::Serialization::Internal::CombineSchemaHashes(
            result, FE::CompileTimeHash("$base:{{ base.qualified_name }}", {{ 6 + (base.qualified_name|length) }}));
        result = FE::Serialization::Internal::CombineSchemaHashes(
            result, FE::Serialization::GetSchemaHash<{{ base.qualified_name }}>());
        {%- endfor %}
        {%- for field in type.serialization_fields %}
        result = FE::Serialization::Internal::CombineSchemaHashes(
            result, FE::CompileTimeHash("{{ field.name }}", {{ field.name|length }}));
        result = FE::Serialization::Internal::CombineSchemaHashes(
            result, FE::Serialization::GetSchemaHash<decltype({{ field.name }})>());
        {%- endfor %}
        result = FE::Serialization::Internal::CombineSchemaHashes(result, {{ type.serialization_version or 0 }});
        return result;
    }

    uint32_t {{ type.name }}::RTTI_GetSerializationVersion()
    {
        return {{ type.serialization_version or 0 }};
    }
{% endif %}
}
{% endif %}
