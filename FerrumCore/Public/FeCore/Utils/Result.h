#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/RTTI/RTTI.h>
#include <variant>

namespace FE
{
    template<class T>
    struct Err
    {
        T m_value;

        Err(const T& value) // NOLINT(google-explicit-constructor)
            : m_value(value)
        {
        }

        Err(T&& value) // NOLINT(google-explicit-constructor)
            : m_value(value)
        {
        }
    };

    template<class T, class TError>
    class Result final
    {
        std::variant<T, TError> m_data;

        struct CreateFromError
        {
        };

        Result(CreateFromError, const TError& error)
            : m_data(error)
        {
        }

        Result(CreateFromError, TError&& error)
            : m_data(std::move(error))
        {
        }

    public:
        Result() = default;

        Result(const T& value) // NOLINT(google-explicit-constructor)
            : m_data(value)
        {
        }

        Result(T&& value) // NOLINT(google-explicit-constructor)
            : m_data(std::move(value))
        {
        }

        Result(const Err<TError>& error) // NOLINT(google-explicit-constructor)
            : m_data(error.m_value)
        {
        }

        Result(Err<TError>&& error) // NOLINT(google-explicit-constructor)
            : m_data(std::move(error.m_value))
        {
        }

        Result(const Result& other) = default;
        Result(Result&& other) noexcept = default;

        Result& operator=(const Result& other) = default;
        Result& operator=(Result&& other) noexcept = default;

        static Result Ok(const T& value)
        {
            return Result(value);
        }

        static Result Ok(T&& value)
        {
            return Result(std::move(value));
        }

        static Result Err(const TError& error)
        {
            return Result(CreateFromError{}, error);
        }

        static Result Err(TError&& error)
        {
            return Result(CreateFromError{}, std::move(error));
        }

        [[nodiscard]] bool IsOk() const
        {
            return m_data.index() == 0;
        }

        [[nodiscard]] bool IsErr() const
        {
            return !IsOk();
        }

        TError UnwrapErr() const
        {
            return ExpectErr("UnwrapErr() called on OK result");
        }

        TError ExpectErr(const char* msg) const
        {
            FE_CORE_ASSERT(IsErr(), msg);
            return std::get<TError>(m_data);
        }

        [[nodiscard]] explicit operator bool() const
        {
            return IsOk();
        }

        [[nodiscard]] T UnwrapOr(const T& defaultValue) const
        {
            if (IsOk())
            {
                return std::get<T>(m_data);
            }

            return defaultValue;
        }

        [[nodiscard]] T UnwrapOrDefault() const
        {
            if (IsOk())
            {
                return std::get<T>(m_data);
            }

            return {};
        }

        T Unwrap() const
        {
            return Expect("Unwrap() called on error result");
        }

        T Expect(const char* msg) const
        {
            FE_CORE_ASSERT(IsOk(), msg);
            return std::get<T>(m_data);
        }
    };
} // namespace FE
