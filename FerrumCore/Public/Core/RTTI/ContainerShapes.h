#pragma once
#include <Core/RTTI/ReflectionContext.h>
#include <festd/string.h>
#include <festd/variant.h>
#include <festd/vector.h>

namespace FE::Rtti
{
    namespace Internal
    {
        //! Shape of a resizable contiguous container with STL-style insert, erase and resize.
        template<class TContainer>
        struct SequenceShapeBinder
        {
            using Element = typename TContainer::value_type;

            static void Bind(Type& type)
            {
                static const SequenceShape kShape = CreateShape();
                type.SetShape(kShape);
            }

        private:
            static TContainer& Cast(void* value)
            {
                return *static_cast<TContainer*>(value);
            }


            //! Fixed-capacity containers report their limit; growable ones are bounded by memory only.
            static bool CanHold(const TContainer& container, const size_t size)
            {
                if constexpr (requires { container.max_size(); })
                    return size <= container.max_size();
                else
                    return true;
            }


            static SequenceShape CreateShape()
            {
                SequenceShape shape;
                shape.m_element = &ResolveType<Element>();
                shape.m_size = [](const void* value) {
                    return static_cast<uint32_t>(static_cast<const TContainer*>(value)->size());
                };
                shape.m_at = [](void* value, const uint32_t index) -> void* {
                    return Cast(value).data() + index;
                };
                shape.m_erase = [](void* value, const uint32_t index) {
                    TContainer& container = Cast(value);
                    container.erase(container.begin() + index);
                };

                if constexpr (std::is_default_constructible_v<Element>)
                {
                    shape.m_insert = [](void* value, const uint32_t index) {
                        TContainer& container = Cast(value);
                        if (!CanHold(container, container.size() + 1))
                            return false;

                        container.emplace(container.begin() + index);
                        return true;
                    };
                    shape.m_resize = [](void* value, const uint32_t size) {
                        TContainer& container = Cast(value);
                        if (!CanHold(container, size))
                            return false;

                        container.resize(size);
                        return true;
                    };
                }

                return shape;
            }
        };


        template<class TArray, class TElement, uint32_t TCount>
        void BindFixedArrayShape(Type& type)
        {
            static const FixedArrayShape kShape{
                .m_element = &ResolveType<TElement>(),
                .m_count = TCount,
                .m_stride = sizeof(TElement),
            };
            type.SetShape(kShape);
        }


        template<class TVariant, class... TAlternatives>
        struct VariantShapeBinder
        {
            static void Bind(Type& type)
            {
                static const Type* const kAlternatives[] = { &ResolveType<TAlternatives>()... };
                static const VariantShape kShape = CreateShape(kAlternatives, std::index_sequence_for<TAlternatives...>{});
                type.SetShape(kShape);
            }

        private:
            using Emplace = void (*)(void*);

            template<size_t... TIndices>
            static VariantShape CreateShape(const festd::span<const Type* const> alternatives, std::index_sequence<TIndices...>)
            {
                static constexpr Emplace kEmplace[] = { [](void* value) {
                    static_cast<TVariant*>(value)->template emplace<TIndices>();
                }... };

                VariantShape shape;
                shape.m_alternatives = alternatives;
                shape.m_index = [](const void* value) {
                    return static_cast<uint32_t>(static_cast<const TVariant*>(value)->index());
                };
                shape.m_get = [](void* value) {
                    return festd::visit(
                        []<class TAlternative>(TAlternative& alternative) -> void* {
                            return &alternative;
                        },
                        *static_cast<TVariant*>(value));
                };
                shape.m_emplace = [](void* value, const uint32_t index) {
                    FE_Assert(index < sizeof...(TAlternatives));
                    kEmplace[index](value);
                };

                return shape;
            }
        };
    } // namespace Internal


    template<class T, size_t TCount>
    struct Shape<T[TCount]>
    {
        static void Bind(Type& type)
        {
            Internal::BindFixedArrayShape<T[TCount], T, static_cast<uint32_t>(TCount)>(type);
        }
    };


    template<class T, size_t TCount>
    struct Shape<festd::array<T, TCount>>
    {
        static void Bind(Type& type)
        {
            static_assert(sizeof(festd::array<T, TCount>) == sizeof(T) * TCount);
            Internal::BindFixedArrayShape<festd::array<T, TCount>, T, static_cast<uint32_t>(TCount)>(type);
        }
    };


    template<class T, class TAllocator>
    struct Shape<eastl::vector<T, TAllocator>> : Internal::SequenceShapeBinder<eastl::vector<T, TAllocator>>
    {
    };


    template<class T, size_t TCapacity, bool TEnableOverflow, class TAllocator>
    struct Shape<eastl::fixed_vector<T, TCapacity, TEnableOverflow, TAllocator>>
        : Internal::SequenceShapeBinder<eastl::fixed_vector<T, TCapacity, TEnableOverflow, TAllocator>>
    {
    };


    template<class T, unsigned TInlineCapacity, class TAllocator>
    struct Shape<gch::small_vector<T, TInlineCapacity, TAllocator>>
        : Internal::SequenceShapeBinder<gch::small_vector<T, TInlineCapacity, TAllocator>>
    {
    };


    template<class... TAlternatives>
    struct Shape<festd::variant<TAlternatives...>>
        : Internal::VariantShapeBinder<festd::variant<TAlternatives...>, TAlternatives...>
    {
    };


    template<class T>
    struct Shape<Rc<T>>
    {
        static void Bind(Type& type)
        {
            static const DynamicShape kShape{
                .m_base = &ResolveType<T>(),
                .m_type = [](const void* value) -> const Type* {
                    const Rc<T>& object = *static_cast<const Rc<T>*>(value);
                    return object ? &object->RTTI_GetDynamicType() : nullptr;
                },
                .m_value = [](void* value) -> void* {
                    Rc<T>& object = *static_cast<Rc<T>*>(value);
                    return object ? object->RTTI_GetDynamicObject() : nullptr;
                },
                .m_reset = &Reset,
            };
            type.SetShape(kShape);
        }

    private:
        static bool Reset(void* value, const Type* type)
        {
            Rc<T>& object = *static_cast<Rc<T>*>(value);
            if (type == nullptr)
            {
                object.Reset();
                return true;
            }

            if (!type->m_defaultConstructor || !type->m_destructor || !type->m_cast)
                return false;

            void* storage = Memory::DefaultAllocate(type->m_size, type->m_alignment);
            type->m_defaultConstructor(storage);
            T* pointer = static_cast<T*>(type->m_cast(storage, GetTypeID<T>()));
            if (pointer == nullptr)
            {
                type->m_destructor(storage);
                Memory::DefaultFree(storage);
                return false;
            }

            object = Rc<T>(pointer);
            return true;
        }
    };


    template<>
    struct Shape<Any>
    {
        static void Bind(Type& type)
        {
            static constexpr DynamicShape kShape{
                .m_base = nullptr,
                .m_type =
                    [](const void* value) {
                        return static_cast<const Any*>(value)->GetType();
                    },
                .m_value =
                    [](void* value) {
                        return static_cast<Any*>(value)->GetValue();
                    },
                .m_reset =
                    [](void* value, const Type* type) {
                        Any& any = *static_cast<Any*>(value);
                        if (type != nullptr)
                            return any.Emplace(*type);

                        any.Reset();
                        return true;
                    },
            };
            type.SetShape(kShape);
        }
    };


    template<class TBase>
    struct Shape<FE::Internal::StringImpl<TBase>>
    {
        using String = FE::Internal::StringImpl<TBase>;

        static void Bind(Type& type)
        {
            static constexpr StringShape kShape{
                .m_get =
                    [](const void* value) {
                        const String& text = *static_cast<const String*>(value);
                        return std::string_view{ text.data(), text.size() };
                    },
                .m_set =
                    [](void* value, const std::string_view text) {
                        static_cast<String*>(value)->assign(text.data(), static_cast<uint32_t>(text.size()));
                    },
            };
            type.SetShape(kShape);
        }
    };


    template<>
    struct Shape<festd::string_view>
    {
        static void Bind(Type& type)
        {
            static constexpr StringShape kShape{
                .m_get =
                    [](const void* value) {
                        const festd::string_view text = *static_cast<const festd::string_view*>(value);
                        return std::string_view{ text.data(), text.size() };
                    },
                .m_set = nullptr,
            };
            type.SetShape(kShape);
        }
    };


    template<>
    struct Shape<Env::Name>
    {
        static void Bind(Type& type)
        {
            static constexpr StringShape kShape{
                .m_get =
                    [](const void* value) {
                        const Env::Name name = *static_cast<const Env::Name*>(value);
                        return name.IsValid() ? std::string_view{ name.c_str(), name.size() } : std::string_view{};
                    },
                .m_set =
                    [](void* value, const std::string_view text) {
                        *static_cast<Env::Name*>(value) = text.empty() ? Env::Name::kEmpty : Env::Name(text);
                    },
            };
            type.SetShape(kShape);
        }
    };


    template<>
    struct Shape<IO::Path>
    {
        static void Bind(Type& type)
        {
            static constexpr StringShape kShape{
                .m_get =
                    [](const void* value) {
                        const auto& text = static_cast<const IO::Path*>(value)->AsBaseString();
                        return std::string_view{ text.data(), text.size() };
                    },
                .m_set =
                    [](void* value, const std::string_view text) {
                        *static_cast<IO::Path*>(value) =
                            IO::Path(festd::string_view{ text.data(), static_cast<uint32_t>(text.size()) });
                    },
            };
            type.SetShape(kShape);
        }
    };


    template<>
    struct Shape<Uuid>
    {
        static void Bind(Type& type)
        {
            static constexpr HandleShape kShape{
                .m_get =
                    [](const void* value) {
                        return *static_cast<const Uuid*>(value);
                    },
                .m_set =
                    [](void* value, const Uuid uuid) {
                        *static_cast<Uuid*>(value) = uuid;
                    },
            };
            type.SetShape(kShape);
        }
    };
} // namespace FE::Rtti
