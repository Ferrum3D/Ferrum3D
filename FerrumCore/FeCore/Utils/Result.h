#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/RTTI/RTTI.h>
#include <variant>

namespace FE
{
    template<class T>
    struct Err
    {
        T Value;

        inline Err(const T& value) // NOLINT(google-explicit-constructor)
            : Value(value)
        {
        }

        inline Err(T&& value) // NOLINT(google-explicit-constructor)
            : Value(value)
        {
        }
    };

    template<class T, class TError>
    class Result final
    {
        std::variant<T, TError> m_Data;

        struct CreateFromError
        {
        };

        inline Result(CreateFromError, const TError& error)
            : m_Data(error)
        {
        }

        inline Result(CreateFromError, TError&& error)
            : m_Data(std::move(error))
        {
        }

    public:
        FE_STRUCT_RTTI(Result, "87C787E0-D455-4480-8BB8-E7ACAD4738ED");

        inline Result() = default;

        inline Result(const T& value) // NOLINT(google-explicit-constructor)
            : m_Data(value)
        {
        }

        inline Result(T&& value) // NOLINT(google-explicit-constructor)
            : m_Data(std::move(value))
        {
        }

        inline Result(const Err<TError>& error) // NOLINT(google-explicit-constructor)
            : m_Data(error.Value)
        {
        }

        inline Result(Err<TError>&& error) // NOLINT(google-explicit-constructor)
            : m_Data(std::move(error.Value))
        {
        }

        inline Result(const Result& other)     = default;
        inline Result(Result&& other) noexcept = default;

        inline Result& operator=(const Result& other)     = default;
        inline Result& operator=(Result&& other) noexcept = default;

        inline static Result Ok(const T& value)
        {
            return Result(value);
        }

        inline static Result Ok(T&& value)
        {
            return Result(std::move(value));
        }

        inline static Result Err(const TError& error)
        {
            return Result(CreateFromError{}, error);
        }

        inline static Result Err(TError&& error)
        {
            return Result(CreateFromError{}, std::move(error));
        }

        [[nodiscard]] inline bool IsOk() const
        {
            return m_Data.index() == 0;
        }

        [[nodiscard]] inline bool IsErr() const
        {
            return !IsOk();
        }

        inline TError UnwrapErr() const
        {
            return ExpectErr("UnwrapErr() called on OK result");
        }

        inline TError ExpectErr(const char* msg) const
        {
            FE_CORE_ASSERT(IsErr(), msg);
            return std::get<TError>(m_Data);
        }

        [[nodiscard]] inline explicit operator bool() const
        {
            return IsOk();
        }

        [[nodiscard]] inline T UnwrapOr(const T& defaultValue) const
        {
            if (IsOk())
            {
                return std::get<T>(m_Data);
            }

            return defaultValue;
        }

        [[nodiscard]] inline T UnwrapOrDefault() const
        {
            if (IsOk())
            {
                return std::get<T>(m_Data);
            }

            return {};
        }

        inline T Unwrap() const
        {
            return Expect("Unwrap() called on error result");
        }

        inline T Expect(const char* msg) const
        {
            FE_CORE_ASSERT(IsOk(), msg);
            return std::get<T>(m_Data);
        }

        template<class F>
        inline constexpr auto OrElse(F&& f) const& -> Result<T, TError>
        {
            using ResultType = Result<T, TError>;

            return IsOk() ? ResultType::Ok(std::get<T>(m_Data))
                          : ResultType::Err(std::invoke(std::forward<F>(f), std::get<TError>(m_Data)));
        }

        template<class F>
        inline constexpr auto OrElse(F&& f) && -> Result<T, TError>
        {
            using ResultType = Result<T, TError>;

            return IsOk() ? ResultType::Ok(static_cast<T&&>(std::get<T>(m_Data)))
                          : ResultType::Err(std::invoke(std::forward<F>(f), static_cast<TError&&>(std::get<TError>(m_Data))));
        }

        template<class F>
        inline constexpr auto AndThen(F&& f) const& -> Result<T, TError>
        {
            using ResultType = Result<T, TError>;

            return IsOk() ? ResultType::Ok(std::invoke(std::forward<F>(f), std::get<T>(m_Data)))
                          : ResultType::Err(std::get<TError>(m_Data));
        }

        template<class F>
        inline constexpr auto AndThen(F&& f) && -> Result<T, TError>
        {
            using ResultType = Result<T, TError>;

            return IsOk() ? ResultType::Ok(std::invoke(std::forward<F>(f), static_cast<T&&>(std::get<T>(m_Data))))
                          : ResultType::Err(static_cast<TError&&>(std::get<TError>(m_Data)));
        }

        template<class F>
        [[nodiscard]] inline constexpr auto Map(F&& f) const& -> Result<std::invoke_result_t<F, const T&>, TError>
        {
            using InvokeResultType = std::invoke_result_t<F, const T&>;
            using ResultType       = Result<InvokeResultType, TError>;

            return IsOk() ? ResultType::Ok(std::invoke(std::forward<F>(f), std::get<T>(m_Data)))
                          : ResultType::Err(std::get<TError>(m_Data));
        }

        template<class F>
        [[nodiscard]] inline constexpr auto Map(F&& f) && -> Result<std::invoke_result_t<F, T&&>, TError>
        {
            using InvokeResultType = std::invoke_result_t<F, T&&>;
            using ResultType       = Result<InvokeResultType, TError>;

            return IsOk() ? ResultType::Ok(std::invoke(std::forward<F>(f), static_cast<T&&>(std::get<T>(m_Data))))
                          : ResultType::Err(static_cast<TError&&>(std::get<TError>(m_Data)));
        }
    };

#define FE_CHECK_RESULT(...)                                                                                                     \
    do                                                                                                                           \
    {                                                                                                                            \
        if ((__VA_ARGS__).IsErr())                                                                                               \
        {                                                                                                                        \
            return ::FE::Err((__VA_ARGS__).UnwrapErr());                                                                         \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)
} // namespace FE
