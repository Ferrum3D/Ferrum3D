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


namespace FE::RTTI
{
    using TypeID = UUID;

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


        template<class TEnum>
        Type* GetEnumTypeMutable() = delete;

        template<class TEnum>
        const TypeID& GetEnumTypeID() = delete;


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
    template<class T_RTTI, std::enable_if_t<FE::RTTI::Internal::kIsRTTIDefined<T_RTTI>, bool>>                                   \
    friend const FE::RTTI::Type& FE::RTTI::GetType();                                                                            \
                                                                                                                                 \
    template<class TDstPtr_RTTI, class TSrc_RTTI, FE::RTTI::Internal::EnableDynCast<TDstPtr_RTTI, TSrc_RTTI>>                    \
    friend TDstPtr_RTTI FE::RTTI::Cast(TSrc_RTTI* source);                                                                       \
    template<class TDstPtr_RTTI, class TSrc_RTTI, FE::RTTI::Internal::EnableDynCast<TDstPtr_RTTI, TSrc_RTTI>>                    \
    friend TDstPtr_RTTI FE::RTTI::AssertCast(TSrc_RTTI* source);                                                                 \
                                                                                                                                 \
    FE_PUSH_CLANG_WARNING("-Winconsistent-missing-override")                                                                     \
    virtual void* FE_VECTORCALL RTTI_TryCast(FE::RTTI::TypeID typeID);                                                           \
    virtual const void* FE_VECTORCALL RTTI_TryCast(FE::RTTI::TypeID typeID) const;                                               \
    FE_POP_CLANG_WARNING()                                                                                                       \
                                                                                                                                 \
public:                                                                                                                          \
    static const FE::RTTI::TypeID TypeID;                                                                                        \
    static FE_CODEGEN_ATTRIBUTE(#codegenAttribute "=" uuid) void Reflect(FE::RTTI::ReflectionContext& context)


#define FE_RTTI(uuid) FE_RTTI_IMPL_POLYMORPHIC(uuid, ReflectBasic)

#define FE_RTTI_Reflect_2(typename, uuid)                                                                                        \
    template<>                                                                                                                   \
    const FE::RTTI::Type& FE::RTTI::GetType<typename>();                                                                         \
    template<>                                                                                                                   \
    FE::RTTI::TypeID FE::RTTI::GetTypeID<typename>();                                                                            \
    template<>                                                                                                                   \
    struct FE_CODEGEN_ATTRIBUTE("ReflectFull=" uuid) FE::RTTI::Internal::ExternalTypeReflector<typename>                         \
    {                                                                                                                            \
        static void Reflect(FE::RTTI::ReflectionContext& context);                                                               \
    }

#define FE_RTTI_Reflect_1(uuid) FE_RTTI_IMPL_POLYMORPHIC(uuid, ReflectFull)

#define FE_RTTI_Reflect(...) FE_MACRO_SPECIALIZE(FE_RTTI_Reflect, __VA_ARGS__)


    template<class T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
    const Type& GetType()
    {
        return *Internal::GetEnumTypeMutable<T>();
    }


    template<class T, std::enable_if_t<Internal::kIsRTTIDefined<T>, bool> = true>
    const Type& GetType()
    {
        return T::RTTI_GetTypeMutable();
    }

    template<class T, std::enable_if_t<!Internal::kIsRTTIDefined<T> && !std::is_enum_v<T>, bool> = true>
    const Type& GetType() = delete;


    template<class T>
    FE_FORCE_INLINE TypeID GetTypeID()
    {
        if constexpr (std::is_enum_v<T>)
        {
            return Internal::GetEnumTypeID<T>();
        }
        else
        {
            if constexpr (Internal::kIsRTTIDefined<T>)
                return T::TypeID;
            else
                return TypeID::kNull;
        }
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
    //! Works just like `RTTI::Cast`, except it will assert that dynamic cast is possible and will not return `nullptr`
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
} // namespace FE::RTTI
