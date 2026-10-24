#pragma once
#include <Core/RTTI/Any.h>
#include <Core/RTTI/Reflection.h>
#include <Core/RTTI/Shape.h>
#include <Core/Serialization/Serialization.h>
#include <memory>

namespace FE::Rtti
{
    template<class T>
    void BindTypeSerialization(Type& type)
    {
        if constexpr (requires(const T& value) {
                          { value.Validate() } -> std::same_as<bool>;
                      })
            type.m_validate = [](const void* value) {
                return static_cast<const T*>(value)->Validate();
            };

        if constexpr (requires(T& value, const typename T::Configuration& configuration) {
                          { value.Configure(configuration) } -> std::same_as<bool>;
                          { std::as_const(value).CaptureConfiguration() } -> std::same_as<typename T::Configuration>;
                      })
        {
            type.m_configure = [](void* storage, const Any& configuration) {
                if (!configuration.HasValue())
                    return static_cast<T*>(storage)->Configure(typename T::Configuration{});
                const auto* typed = configuration.TryGet<typename T::Configuration>();
                return typed != nullptr && static_cast<T*>(storage)->Configure(*typed);
            };
            type.m_captureConfiguration = [](const void* storage) {
                Any configuration;
                configuration.Emplace<typename T::Configuration>(static_cast<const T*>(storage)->CaptureConfiguration());
                return configuration;
            };
            if constexpr (requires(T& value, const typename T::Configuration& configuration) {
                              { value.ApplyConfiguration(configuration) } -> std::same_as<bool>;
                          })
            {
                type.m_applyConfiguration = [](void* storage, const Any& configuration) {
                    const auto* typed = configuration.TryGet<typename T::Configuration>();
                    return typed != nullptr && static_cast<T*>(storage)->ApplyConfiguration(*typed);
                };
            }
        }

        using namespace Serialization;
        if constexpr (Serialization::Internal::HasSerializer<T>)
        {
            type.m_serialize = [](SerializationContext& context, const void* value) {
                return SerializeValue(context, *static_cast<const T*>(value));
            };
            type.m_deserialize = [](DeserializationContext& context, void* value) {
                return DeserializeValue(context, *static_cast<T*>(value));
            };
            type.m_serializationVersion = GetVersion<T>();
            type.m_serializationSchemaHash = GetSchemaHash<T>();
        }
    }


    namespace Internal
    {
        template<class T>
        concept IsExternallyReflected = requires { &ExternalTypeReflector<T>::Reflect; };


        template<class T>
        void BindArrayLifecycle(Type& type)
        {
            using Element = std::remove_all_extents_t<T>;
            constexpr size_t kCount = sizeof(T) / sizeof(Element);
            if constexpr (std::is_default_constructible_v<Element>)
            {
                type.m_defaultConstructor = [](void* storage) {
                    std::uninitialized_value_construct_n(static_cast<Element*>(storage), kCount);
                };
            }
            if constexpr (std::is_copy_constructible_v<Element>)
            {
                type.m_copyConstructor = [](void* storage, const void* source) {
                    std::uninitialized_copy_n(static_cast<const Element*>(source), kCount, static_cast<Element*>(storage));
                };
            }
            if constexpr (std::is_copy_assignable_v<Element>)
            {
                type.m_copyAssignment = [](void* storage, const void* source) {
                    std::copy_n(static_cast<const Element*>(source), kCount, static_cast<Element*>(storage));
                };
            }
            if constexpr (std::is_move_constructible_v<Element>)
            {
                type.m_moveConstructor = [](void* storage, void* source) {
                    std::uninitialized_move_n(static_cast<Element*>(source), kCount, static_cast<Element*>(storage));
                };
            }

            type.m_destructor = [](void* storage) {
                std::destroy_n(static_cast<Element*>(storage), kCount);
            };
        }


        template<class T>
        void BindTypeLifecycle(Type& type)
        {
            type.m_defaultConstructor = nullptr;
            type.m_copyConstructor = nullptr;
            type.m_copyAssignment = nullptr;
            type.m_moveConstructor = nullptr;
            type.m_destructor = nullptr;
            // Placement array new may add a cookie, so arrays construct their elements individually.
            if constexpr (std::is_array_v<T>)
            {
                BindArrayLifecycle<T>(type);
            }
            else
            {
                if constexpr (std::is_move_constructible_v<T>)
                {
                    type.m_moveConstructor = [](void* storage, void* source) {
                        ::new (storage) T(std::move(*static_cast<T*>(source)));
                    };
                }
                if constexpr (std::is_default_constructible_v<T>)
                {
                    type.m_defaultConstructor = [](void* storage) {
                        ::new (storage) T();
                    };
                }
                if constexpr (std::is_copy_constructible_v<T>)
                {
                    type.m_copyConstructor = [](void* storage, const void* source) {
                        ::new (storage) T(*static_cast<const T*>(source));
                    };
                }
                if constexpr (std::is_copy_assignable_v<T>)
                {
                    type.m_copyAssignment = [](void* storage, const void* source) {
                        *static_cast<T*>(storage) = *static_cast<const T*>(source);
                    };
                }
                if constexpr (std::is_destructible_v<T>)
                {
                    type.m_destructor = [](void* storage) {
                        static_cast<T*>(storage)->~T();
                    };
                }
            }
        }


        template<class T>
        consteval ScalarKind GetScalarKind()
        {
            if constexpr (std::is_same_v<T, bool>)
                return ScalarKind::kBool;
            else if constexpr (std::is_floating_point_v<T>)
                return ScalarKind::kFloat;
            else if constexpr (std::is_signed_v<T>)
                return ScalarKind::kSigned;
            else
                return ScalarKind::kUnsigned;
        }


        //! Apply the Shape specialization of T, or the shape implied by its kind; reflected classes default to kStruct.
        template<class T>
        void BindTypeShape(Type& type)
        {
            if constexpr (HasShape<T>)
            {
                Shape<T>::Bind(type);
            }
            else if constexpr (std::is_enum_v<T>)
            {
                using Underlying = std::underlying_type_t<T>;
                static constexpr EnumShape kShape{
                    .m_get =
                        [](const void* value) {
                            return static_cast<int64_t>(static_cast<Underlying>(*static_cast<const T*>(value)));
                        },
                    .m_set =
                        [](void* value, const int64_t underlying) {
                            *static_cast<T*>(value) = static_cast<T>(static_cast<Underlying>(underlying));
                        },
                };
                type.SetShape(kShape);
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                static constexpr ScalarShape kShape{ .m_kind = GetScalarKind<T>() };
                type.SetShape(kShape);
            }
        }


        template<class T>
        void BindTypeLayout(Type& type)
        {
            type.m_size = sizeof(T);
            type.m_alignment = alignof(T);
            if (std::is_trivial_v<T>)
                type.m_flags |= TypeFlags::kTrivial;
            if (std::is_standard_layout_v<T>)
                type.m_flags |= TypeFlags::kStandardLayout;
        }


        //! @brief Type of a C++ type without generated reflection, created and registered on first use.
        //!
        //! Its ID is derived from the compiler's type name, so it is stable within one toolchain only and must not be persisted.
        template<class T>
        struct SynthesizedType final
        {
            static const Type& Get()
            {
                static Type& type = Create();
                return type;
            }

        private:
            static Type& Create()
            {
                static Type type;
                static constexpr uint64_t kSalt = CompileTimeHash("FE::Rtti::SynthesizedType");
                type.m_id = TypeID(_mm_set_epi64x(static_cast<int64_t>(TypeNameHash<T>),
                                                  static_cast<int64_t>(HashCombine(TypeNameHash<T>, kSalt))));
                type.m_qualifiedName = festd::ascii_view(TypeName<T>.data(), static_cast<uint32_t>(TypeName<T>.size()));
                type.m_name = type.m_qualifiedName;
                type.m_flags = TypeFlags::kSynthesized;
                if constexpr (std::is_enum_v<T>)
                    type.m_flags |= TypeFlags::kEnum;

                BindTypeLayout<T>(type);
                BindTypeLifecycle<T>(type);
                BindTypeSerialization<T>(type);
                BindTypeShape<T>(type);
                TypeRegistry::Internal::RegisterSynthesizedType(type);
                return type;
            }
        };
    } // namespace Internal


    //! @brief Type of any C++ type: generated reflection when it exists, otherwise a synthesized type.
    //!
    //! Field types are resolved while their classes are reflected. A synthesized type first resolved later registers then,
    //! and the registry is not synchronized, so do that on the main thread.
    template<class T>
    const Type& ResolveType()
    {
        using Value = std::remove_cv_t<T>;
        if constexpr (Internal::kIsRTTIDefined<Value> || Internal::IsExternallyReflected<Value>)
            return GetType<Value>();
        else
            return Internal::SynthesizedType<Value>::Get();
    }


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
                          const festd::span<const FieldInfo> fields, const TypeFlags extraFlags = TypeFlags::kNone)
        {
            type.m_id = id;
            type.m_name = GetShortName(qualifiedName);
            type.m_qualifiedName = qualifiedName;

            Internal::BindTypeLifecycle<T>(type);

            type.m_baseTypes = festd::span(reinterpret_cast<const TypeID*>(baseTypes.data()), baseTypes.size() / sizeof(TypeID));
            type.m_attributes = attributes;
            type.m_fields = fields;
            type.m_flags = TypeFlags::kClass | extraFlags;
            Internal::BindTypeLayout<T>(type);

            type.m_shape = TypeShape::kStruct;
            Internal::BindTypeShape<T>(type);
            BindTypeSerialization<T>(type);
            RegisterType(type);
        }


        template<class T>
        void ReflectEnum(Type& type, const TypeID id, const festd::span<const uint8_t> underlyingType,
                         const festd::ascii_view qualifiedName, const festd::span<const Attribute> attributes,
                         const festd::span<const festd::ascii_view> names,
                         const festd::span<const festd::ascii_view> displayNames, const festd::span<const int64_t> values)
        {
            FE_Assert(underlyingType.size() == sizeof(TypeID));

            type.m_id = id;
            type.m_name = GetShortName(qualifiedName);
            type.m_qualifiedName = qualifiedName;
            type.m_baseTypes = festd::span(reinterpret_cast<const TypeID*>(underlyingType.data()), 1);
            type.m_attributes = attributes;
            type.m_enumNames = names;
            type.m_enumDisplayNames = displayNames;
            type.m_enumValues = values;
            type.m_size = sizeof(T);
            type.m_alignment = alignof(T);
            type.m_flags = TypeFlags::kEnum | TypeFlags::kTrivial | TypeFlags::kStandardLayout;

            Internal::BindTypeLifecycle<T>(type);
            Internal::BindTypeShape<T>(type);
            BindTypeSerialization<T>(type);
            RegisterType(type);
        }


        template<class T>
        void ReflectBuiltinType(Type& type, const TypeID id, const festd::ascii_view name)
        {
            type.m_id = id;
            type.m_name = name;
            type.m_qualifiedName = name;
            Internal::BindTypeLayout<T>(type);

            Internal::BindTypeLifecycle<T>(type);
            Internal::BindTypeShape<T>(type);
            BindTypeSerialization<T>(type);
            RegisterType(type);
        }


        template<class TClass, class TField>
        static FieldInfo CreateFieldInfo(const festd::ascii_view name, TField TClass::* field,
                                         const festd::span<const Attribute> attributes, const FieldFlags flags)
        {
            const auto fieldOffset = reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<TClass const volatile*>(0)->*field));

            FieldInfo fieldInfo;
            fieldInfo.m_name = name;
            fieldInfo.m_type = &ResolveType<TField>();
            fieldInfo.m_attributes = attributes;
            fieldInfo.m_offset = static_cast<uint32_t>(fieldOffset);
            fieldInfo.m_size = sizeof(TField);
            fieldInfo.m_flags = flags;

            if constexpr (std::is_pointer_v<TField>)
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
} // namespace FE::Rtti

#include <Core/RTTI/ContainerShapes.h>
