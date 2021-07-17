#pragma once
#include "CoreUtils.h"
#include <Console/FeLog.h>
#include <tuple>
#include <variant>

namespace FE
{
    template<class T, class TError = EmptyStruct, class TOk = EmptyStruct>
    class Result
    {
        using OkVariant = std::tuple<TOk, T>;

        std::variant<OkVariant, TError> m_Data;

        inline Result(OkVariant&& data)
        {
            m_Data = std::move(data);
        }

        inline Result(TError&& data)
        {
            m_Data = std::move(data);
        }

    public:
        template<class... Args>
        inline static Result Ok(const T& value, Args&&... args)
        {
            return Result(std::make_tuple(TOk(std::forward<Args>(args)...), value));
        }

        template<class... Args>
        inline static Result Ok(T&& value, Args&&... args)
        {
            return Result(std::make_tuple(TOk(std::forward<Args>(args)...), std::move(value)));
        }

        template<class... Args>
        inline static Result Err(Args&&... args)
        {
            return Result(TError(std::forward<Args>(args)...));
        }

        inline bool IsOk()
        {
            return m_Data.index() == 0;
        }

        inline operator bool()
        {
            return IsOk();
        }

        inline T Unwrap()
        {
            FE_ASSERT_MSG(IsOk(), "Unwrap() called on error result");
            return std::get<1>(std::get<OkVariant>(m_Data));
        }

        template<class... Args>
        inline T Expect(Args&&... msg)
        {
            FE_ASSERT_MSG(IsOk(), std::forward<Args>(msg)...);
            return std::get<1>(std::get<OkVariant>(m_Data));
        }
    };
} // namespace FE
