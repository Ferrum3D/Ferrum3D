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

        inline Result(const Result& other) = default;
        inline Result(Result&& other) noexcept = default;

        inline Result& operator=(const Result& other) = default;
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
    };
} // namespace FE
