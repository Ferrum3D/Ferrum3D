#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/FeUnicode.h>
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
        inline Result() = default;

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

        inline TError GetError()
        {
            FE_CORE_ASSERT(!IsOk(), "GetError() called on OK result");
            return std::get<TError>(m_Data);
        }

        inline TOk GetOk()
        {
            FE_CORE_ASSERT(IsOk(), "GetOk() called on error result");
            return std::get<0>(std::get<OkVariant>(m_Data));
        }

        inline operator bool()
        {
            return IsOk();
        }

        inline T Unwrap()
        {
            FE_CORE_ASSERT(IsOk(), "Unwrap() called on error result");
            return std::get<1>(std::get<OkVariant>(m_Data));
        }

        template<class... Args>
        inline T Expect(Args&&... msg)
        {
            FE_CORE_ASSERT(IsOk(), "");
            return std::get<1>(std::get<OkVariant>(m_Data));
        }

        template<class... Args>
        inline OkVariant ExpectEx(Args&&... msg)
        {
            FE_CORE_ASSERT(IsOk(), "");
            return std::get<OkVariant>(m_Data);
        }

        /**
         * @brief 
         * @tparam FOk 
         * @tparam FError 
         * @param okAction f(const TOk&, const T&)
         * @param errAction f(const TError&)
         * @return 
         */
        template<class FOk, class FError>
        inline auto Match(FOk&& okAction, FError&& errAction) -> typename std::invoke_result<FError, const TError&>::type
        {
            using OkReturn  = typename std::invoke_result<FOk, const TOk&, const T&>::type;
            using ErrReturn = typename std::invoke_result<FError, const TError&>::type;
            static_assert(std::is_same_v<OkReturn, ErrReturn>, "Both actions must return the same type");

            if (IsOk())
            {
                const auto& [res, data] = std::get<OkVariant>(m_Data);
                if constexpr (std::is_same_v<OkReturn, void>)
                    okAction(res, data);
                else
                    return okAction(res, data);
            }
            else
            {
                if constexpr (std::is_same_v<OkReturn, void>)
                    errAction(std::get<TError>(m_Data));
                else
                    return errAction(std::get<TError>(m_Data));
            }
        }

        /**
         * @brief 
         * @tparam F 
         * @param action f(const TOk&, const T&)
         * @return 
        */
        template<class F>
        inline auto OnOk(F&& action) -> typename std::invoke_result<F, const TOk&, const T&>::type
        {
            return Match(std::forward<F>(action), [](auto) {});
        }

        /**
         * @brief 
         * @tparam F 
         * @param action f(const TError&)
         * @return 
        */
        template<class F>
        inline auto OnErr(F&& action) -> typename std::invoke_result<F, const TError&>::type
        {
            return Match([](auto) {}, std::forward<F>(action));
        }
    };
} // namespace FE
