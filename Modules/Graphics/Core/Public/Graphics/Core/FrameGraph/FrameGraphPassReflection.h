#pragma once
#include <type_traits>

namespace FE::Graphics::Core
{
    namespace Internal
    {
        template<class TStruct, class = void, class... T>
        struct IsInitializableImpl final : std::false_type
        {
        };


        template<class TStruct, class... T>
        struct IsInitializableImpl<TStruct, std::void_t<decltype(TStruct{ std::declval<T>()... })>, T...> final : std::true_type
        {
        };


        template<class TStruct, class... T>
        constexpr bool kIsInitializable = IsInitializableImpl<TStruct, void, T...>::value;


        struct ConvertibleToAnything
        {
            template<class T>
            operator T() const
            {
                return T();
            }
        };


        template<class TStruct>
        struct StructFieldCountHelper final
        {
        };
    } // namespace Internal

} // namespace FE::Graphics::Core
