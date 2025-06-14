#pragma once
#include <FeCore/Base/BaseTypes.h>

#include <EASTL/array.h>
#include <EASTL/finally.h>
#include <EASTL/optional.h>
#include <EASTL/sort.h>
#include <string_view>
#include <tl/expected.hpp>

namespace FE::festd
{
    using ascii_view = std::string_view;

    using eastl::array;

    using tl::expected;
    using tl::unexpected;

    using tl::in_place;
    using tl::unexpect;

    using eastl::nullopt;
    using eastl::optional;


    //
    // We redeclare std::move and std::forward here because the STL versions are not inlined
    // in non-optimized builds with MSVC even with /d2Obforceinline.
    // So we have to explicitly mark these __forceinline
    //


    template<typename T>
    FE_ALWAYS_INLINE constexpr std::remove_reference_t<T>&& move(T&& x) noexcept
    {
        return static_cast<std::remove_reference_t<T>&&>(x);
    }


    template<typename T>
    FE_ALWAYS_INLINE constexpr T&& forward(std::remove_reference_t<T>& x) noexcept
    {
        return static_cast<T&&>(x);
    }


    template<typename T>
    FE_ALWAYS_INLINE constexpr T&& forward(std::remove_reference_t<T>&& x) noexcept
    {
        static_assert(!std::is_lvalue_reference_v<T>);
        return static_cast<T&&>(x);
    }


    template<typename TFunc>
    [[nodiscard]] auto defer(TFunc&& f)
    {
        return eastl::finally<TFunc>(forward<TFunc>(f));
    }


    //! @brief Converts an enum value to its underlying type.
    template<class T>
    constexpr std::underlying_type_t<T> to_underlying(T value)
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }


    template<class TTo, class TFrom>
    constexpr TTo bit_cast(const TFrom& value)
    {
        return __builtin_bit_cast(TTo, value);
    }


    using eastl::begin;
    using eastl::end;
    using eastl::sort;
    using eastl::swap;

    using eastl::copy;
    using eastl::copy_if;
    using eastl::copy_n;


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
        return static_cast<std::conditional_t<TSize <= UINT32_MAX, uint32_t, size_t>>(TSize);
    }


    //! @brief Returns the size of a container in bytes.
    template<class TContainer>
    constexpr auto size_bytes(const TContainer& container) -> decltype(container.size())
    {
        return container.size() * sizeof(*festd::begin(container));
    }


    //! @brief Returns the size of an array in bytes.
    template<class T, size_t TSize>
    constexpr std::conditional_t<TSize * sizeof(T) <= UINT32_MAX, uint32_t, size_t> size_bytes(const T (&)[TSize]) noexcept
    {
        return static_cast<std::conditional_t<TSize * sizeof(T) <= UINT32_MAX, uint32_t, size_t>>(TSize * sizeof(T));
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

    template<class TContainer, class T>
    uint32_t lower_bound_index(TContainer& container, const T& value)
    {
        const auto iter = eastl::lower_bound(begin(container), end(container), value);
        return static_cast<uint32_t>(iter - begin(container));
    }

    template<class TContainer, class T, class TFunctor>
    uint32_t lower_bound_index(TContainer& container, const T& value, TFunctor compare)
    {
        const auto iter = eastl::lower_bound(begin(container), end(container), value, compare);
        return static_cast<uint32_t>(iter - begin(container));
    }


    template<class TContainer, class T>
    auto upper_bound(TContainer& container, const T& value) -> decltype(begin(container))
    {
        return eastl::upper_bound(begin(container), end(container), value);
    }

    template<class TContainer, class T, class TFunctor>
    auto upper_bound(TContainer& container, const T& value, TFunctor compare) -> decltype(begin(container))
    {
        return eastl::upper_bound(begin(container), end(container), value, compare);
    }

    template<class TContainer, class T>
    uint32_t upper_bound_index(TContainer& container, const T& value)
    {
        const auto iter = eastl::upper_bound(begin(container), end(container), value);
        return static_cast<uint32_t>(iter - begin(container));
    }

    template<class TContainer, class T, class TFunctor>
    uint32_t upper_bound_index(TContainer& container, const T& value, TFunctor compare)
    {
        const auto iter = eastl::upper_bound(begin(container), end(container), value, compare);
        return static_cast<uint32_t>(iter - begin(container));
    }


    template<class TIter, class TFunctor>
    uint32_t find_index_if(const TIter& begin, const TIter& end, TFunctor predicate)
    {
        auto iter = eastl::find_if(begin, end, predicate);
        return iter == end ? kInvalidIndex : static_cast<uint32_t>(iter - begin);
    }


    template<class TContainer, class TFunctor>
    uint32_t find_index_if(const TContainer& container, TFunctor predicate)
    {
        return find_index_if(begin(container), end(container), predicate);
    }


    template<class TIter, class T>
    uint32_t find_index(const TIter& begin, const TIter& end, const T& value)
    {
        auto iter = eastl::find(begin, end, value);
        return iter == end ? kInvalidIndex : static_cast<uint32_t>(iter - begin);
    }


    template<class TContainer, class T>
    uint32_t find_index(const TContainer& container, const T& value)
    {
        return find_index(begin(container), end(container), value);
    }
} // namespace FE::festd
