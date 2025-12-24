#pragma once
#include <FeCore/Base/Hash.h>
#include <FeCore/Math/UUID.h>
#include <festd/base.h>
#include <type_traits>


#if FE_CODEGEN
#    define FE_CODEGEN_ATTRIBUTE(value) __attribute__((annotate(value)))
#else
#    define FE_CODEGEN_ATTRIBUTE(value)
#endif

#define FE_ATTRIBUTE(value) FE_CODEGEN_ATTRIBUTE(#value)


namespace FE::Rtti
{
    using TypeID = Uuid;

    struct Attribute;
    struct FieldInfo;
    struct Type;
    struct TypeList;
    struct ReflectionContext;


    namespace Internal
    {
        template<class T>
        struct ExternalTypeReflector
        {
        };


        template<class TDst, class TSrc>
        inline constexpr bool kIsDynamicCastAllowed =
            std::is_base_of_v<TSrc, TDst> || std::is_base_of_v<TDst, TSrc> || std::is_same_v<TDst, TSrc>;

        template<class TDstPtr, class TSrc>
        using EnableDynCast = std::enable_if_t<kIsDynamicCastAllowed<std::remove_pointer_t<TDstPtr>, TSrc>, bool>;


        template<class T>
        using RTTIDefinedType = decltype(T::TypeID);

        template<class T>
        inline constexpr bool kIsRTTIDefined = festd::detect_v<T, RTTIDefinedType>;
    } // namespace Internal


#define FE_RTTI_IMPL_POLYMORPHIC(uuid, codegenAttribute)                                                                         \
private:                                                                                                                         \
    template<class T_RTTI, std::enable_if_t<FE::Rtti::Internal::kIsRTTIDefined<T_RTTI>, bool>>                                   \
    friend const FE::Rtti::Type& FE::Rtti::GetType();                                                                            \
                                                                                                                                 \
    template<class TDstPtr_RTTI, class TSrc_RTTI, FE::Rtti::Internal::EnableDynCast<TDstPtr_RTTI, TSrc_RTTI>>                    \
    friend TDstPtr_RTTI FE::Rtti::Cast(TSrc_RTTI* source);                                                                       \
    template<class TDstPtr_RTTI, class TSrc_RTTI, FE::Rtti::Internal::EnableDynCast<TDstPtr_RTTI, TSrc_RTTI>>                    \
    friend TDstPtr_RTTI FE::Rtti::AssertCast(TSrc_RTTI* source);                                                                 \
    template<class T_RTTI>                                                                                                       \
    friend bool FE::Rtti::IsDerivedFrom(T_RTTI* instance, const FE::Rtti::TypeID typeID);                                        \
                                                                                                                                 \
    static const FE::Rtti::Type& RTTI_GetType();                                                                                 \
                                                                                                                                 \
    FE_PUSH_CLANG_WARNING("-Winconsistent-missing-override")                                                                     \
    virtual void* FE_VECTORCALL RTTI_TryCast(FE::Rtti::TypeID typeID);                                                           \
    virtual const void* FE_VECTORCALL RTTI_TryCast(FE::Rtti::TypeID typeID) const;                                               \
    FE_POP_CLANG_WARNING()                                                                                                       \
                                                                                                                                 \
public:                                                                                                                          \
    static const FE::Rtti::TypeID TypeID;                                                                                        \
    static FE_CODEGEN_ATTRIBUTE(#codegenAttribute "=" uuid) void Reflect(FE::Rtti::ReflectionContext& context)


//! @brief Macro to register a class to the RTTI system without reflecting it.
//!
//! This macro enables `Rtti::Cast` and `Rtti::AssertCast`. Use this inside a polymorphic class definition.
#define FE_RTTI(uuid) FE_RTTI_IMPL_POLYMORPHIC(uuid, ReflectBasic)

#define FE_RTTI_Reflect_2(typename, uuid)                                                                                        \
    template<>                                                                                                                   \
    const FE::Rtti::Type& FE::Rtti::GetType<typename>();                                                                         \
    template<>                                                                                                                   \
    FE::Rtti::TypeID FE::Rtti::GetTypeID<typename>();                                                                            \
    template<>                                                                                                                   \
    struct FE_CODEGEN_ATTRIBUTE("ReflectFull=" uuid) FE::Rtti::Internal::ExternalTypeReflector<typename>                         \
    {                                                                                                                            \
        static_assert(!std::is_polymorphic_v<typename>);                                                                         \
        static void Reflect(FE::Rtti::ReflectionContext& context);                                                               \
    }

#define FE_RTTI_Reflect_1(uuid) FE_RTTI_IMPL_POLYMORPHIC(uuid, ReflectFull)

//! @brief Macro to reflect a class to the RTTI system.
//!
//! Two variants are available:
//!
//! - `FE_RTTI_Reflect(uuid)` - version with one argument, uuid is a string literal. Use this inside a polymorphic
//!   class definition.
//!
//! - `FE_RTTI_Reflect(type, uuid)` - version with two arguments, type is a fully qualified class name, uuid is a string literal.
//!   Use this in the global scope to reflect external non-polymorphic types.
//!
//! @note `uuid` can either be a string literal specifying a UUID or "Random" to generate a random UUID each time
//!       the reflection codegen is run.
#define FE_RTTI_Reflect(...) FE_MACRO_SPECIALIZE(FE_RTTI_Reflect, __VA_ARGS__)


    template<class T, std::enable_if_t<Internal::kIsRTTIDefined<T>, bool> = true>
    const Type& GetType()
    {
        return T::RTTI_GetType();
    }

    template<class T, std::enable_if_t<!Internal::kIsRTTIDefined<T>, bool> = true>
    const Type& GetType() = delete;


    template<class T>
    FE_FORCE_INLINE TypeID GetTypeID()
    {
        if constexpr (Internal::kIsRTTIDefined<T>)
            return T::TypeID;
        else
            return TypeID::kNull;
    }


    //! @brief Check if source is derived from or is an instance of given type, i.e. `Rtti::Cast` would succeed.
    template<class T>
    bool IsDerivedFrom(T* source, const TypeID typeID)
    {
        return source->RTTI_TryCast(typeID) != nullptr;
    }


    //! @brief Perform a dynamic cast between pointers to classes up or down the inheritance chain.
    template<class TDstPtr, class TSrc, Internal::EnableDynCast<TDstPtr, TSrc> = true>
    TDstPtr Cast(TSrc* source)
    {
        if (source == nullptr)
            return nullptr;

        using DestinationClass = std::remove_pointer_t<TDstPtr>;
        if (auto* dest = source->RTTI_TryCast(DestinationClass::TypeID))
            return static_cast<TDstPtr>(dest);

        return nullptr;
    }


    //! @brief Perform a static cast between pointers checking its safety in debug builds.
    //!
    //! Works just like `Rtti::Cast`, except it will assert that dynamic cast is possible and will not return `nullptr`
    //! unless `source` is `nullptr`. Use this when you're certainly sure that you can use `static_cast` here, but want to check it
    //! in debug builds. In release builds, the behavior of invalid cast is undefined.
    template<class TDstPtr, class TSrc, Internal::EnableDynCast<TDstPtr, TSrc> = true>
    TDstPtr AssertCast(TSrc* source)
    {
        if (source == nullptr)
            return nullptr;

        if (Build::IsDebug())
        {
            using DestinationClass = std::remove_pointer_t<TDstPtr>;
            auto* dest = source->RTTI_TryCast(DestinationClass::TypeID);
            FE_CoreAssert(static_cast<TDstPtr>(dest) == static_cast<TDstPtr>(source), "AssertCast failed");
        }

        return static_cast<TDstPtr>(source);
    }
} // namespace FE::Rtti


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
