#pragma once
#include <EASTL/finally.h>
#include <EASTL/optional.h>
#include <EASTL/sort.h>
#include <string_view>
#include <tl/expected.hpp>

namespace FE::festd
{
    using ascii_view = std::string_view;

    using tl::expected;
    using tl::unexpected;

    using tl::in_place;
    using tl::unexpect;

    using eastl::nullopt;
    using eastl::optional;


    template<typename TFunc>
    [[nodiscard]] auto defer(TFunc&& f)
    {
        return eastl::finally<TFunc>(std::forward<TFunc>(f));
    }


    //! @brief Converts an enum value to its underlying type.
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


    //! @brief Returns the size of a container.
    template<class TContainer>
    constexpr auto size(const TContainer& container) -> decltype(container.size())
    {
        return container.size();
    }


    //! @brief Returns the size of an array.
    template<class T, size_t TSize>
    constexpr std::conditional_t<TSize <= UINT32_MAX, uint32_t, size_t> size(const T (&)[TSize]) noexcept
    {
        return static_cast<uint32_t>(TSize);
    }


    //! @brief Sorts a container using a comparison function.
    template<class TContainer, class TFunctor>
    void sort(TContainer& container, TFunctor compare)
    {
        eastl::sort(begin(container), end(container), compare);
    }


    template<class TContainer, class T>
    auto lower_bound(TContainer& container, const T& value) -> decltype(begin(container))
    {
        return eastl::lower_bound(begin(container), end(container), value);
    }


    template<class TContainer, class T, class TFunctor>
    auto lower_bound(TContainer& container, const T& value, TFunctor compare) -> decltype(begin(container))
    {
        return eastl::lower_bound(begin(container), end(container), value, compare);
    }
} // namespace FE::festd
