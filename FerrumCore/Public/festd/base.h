#pragma once
#include <EASTL/finally.h>
#include <EASTL/sort.h>
#include <string_view>

namespace FE::festd
{
    using ascii_view = std::string_view;


    template<typename TFunc>
    [[nodiscard]] auto defer(TFunc&& f)
    {
        return eastl::finally<TFunc>(std::forward<TFunc>(f));
    }


    template<class T>
    constexpr std::underlying_type_t<T> to_underlying(T value)
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }


    template<class TTo, class TFrom>
    constexpr std::enable_if_t<std::is_default_constructible_v<TTo> && sizeof(TTo) == sizeof(TFrom), TTo> bit_cast(
        const TFrom& value)
    {
        return __builtin_bit_cast(TTo, value);
    }


    using eastl::begin;
    using eastl::end;
    using eastl::sort;


    template<class TContainer>
    constexpr auto size(const TContainer& container) -> decltype(container.size())
    {
        return container.size();
    }


    template<class T, size_t TSize>
    constexpr std::conditional_t<TSize <= UINT32_MAX, uint32_t, size_t> size(const T (&)[TSize]) noexcept
    {
        return static_cast<uint32_t>(TSize);
    }


    template<class TContainer, class TFunctor>
    void sort(TContainer& container, TFunctor compare)
    {
        eastl::sort(begin(container), end(container), compare);
    }
} // namespace FE::festd
