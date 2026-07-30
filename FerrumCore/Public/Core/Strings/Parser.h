#pragma once
#include <festd/base.h>
#include <festd/string.h>

namespace FE::Parser
{
    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kEmptyString = -1,
        kInvalidFormat = -2,
        kOutOfRange = -3,
        kUnknownError = kDefaultErrorCode<ResultCode>,
    };


    //! @brief Convert a string representation of a number to a 64-bit unsigned integer.
    //!
    //! @param s      The string to parse.
    //! @param result The variable that receives the result of the operation.
    //! @param base   The integer base to use.
    //!
    //! @return The result of the operation.
    ResultCode TryParseUInt64(festd::string_view s, uint64_t& result, int32_t base = 10);


    //! @brief Convert a string representation of a number to a 64-bit signed integer.
    //!
    //! @param s      The string to parse.
    //! @param result The variable that receives the result of the operation.
    //! @param base   The integer base to use.
    //!
    //! @return The result of the operation.
    ResultCode TryParseInt64(festd::string_view s, int64_t& result, int32_t base = 10);


    //! @brief Convert a string representation of a number to a double.
    //!
    //! @param s      The string to parse.
    //! @param result The variable that receives the result of the operation.
    //!
    //! @return The result of the operation.
    ResultCode TryParseDouble(festd::string_view s, double& result);


    template<class T>
    festd::expected<T, ResultCode> TryParse(const festd::string_view s)
    {
        if constexpr (std::is_integral_v<T>)
        {
            if constexpr (std::is_unsigned_v<T>)
            {
                uint64_t result;
                if (const ResultCode resultCode = TryParseUInt64(s, result); resultCode != ResultCode::kSuccess)
                    return festd::unexpected(resultCode);

                if (result > Constants::kMaxValue<T>)
                    return festd::unexpected(ResultCode::kOutOfRange);

                return static_cast<T>(result);
            }
            else if constexpr (std::is_signed_v<T>)
            {
                int64_t result;
                if (const ResultCode resultCode = TryParseInt64(s, result); resultCode != ResultCode::kSuccess)
                    return festd::unexpected(resultCode);

                if (result > Constants::kMaxValue<T> || result < Constants::kMinValue<T>)
                    return festd::unexpected(ResultCode::kOutOfRange);

                return static_cast<T>(result);
            }
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            double result;
            if (const ResultCode resultCode = TryParseDouble(s, result); resultCode != ResultCode::kSuccess)
                return festd::unexpected(resultCode);

            return static_cast<T>(result);
        }
        else
        {
            FE_DebugBreak();
            return festd::unexpected(ResultCode::kUnknownError);
        }
    }


    template<class T>
    T Parse(const festd::string_view s)
    {
        return TryParse<T>(s).value();
    }
} // namespace FE::Parser
