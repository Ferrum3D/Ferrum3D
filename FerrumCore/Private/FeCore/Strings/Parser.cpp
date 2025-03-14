#include <FeCore/Strings/Parser.h>
#include <charconv>

namespace FE::Parser
{
    namespace
    {
        ResultCode ConvertResult(const std::errc ec)
        {
            if (ec == std::errc{})
                return ResultCode::kSuccess;

            switch (ec)
            {
            case std::errc::invalid_argument:
                return ResultCode::kInvalidFormat;
            case std::errc::result_out_of_range:
                return ResultCode::kOutOfRange;
            default:
                return ResultCode::kUnknownError;
            }
        }
    } // namespace


    ResultCode TryParseUInt64(festd::string_view s, uint64_t& result, const int32_t base)
    {
        s = s.strip();
        if (s.empty())
            return ResultCode::kEmptyString;

        const std::from_chars_result r = std::from_chars(s.data(), s.data() + s.size(), result, base);
        return ConvertResult(r.ec);
    }


    ResultCode TryParseInt64(festd::string_view s, int64_t& result, int32_t base)
    {
        s = s.strip();
        if (s.empty())
            return ResultCode::kEmptyString;

        const std::from_chars_result r = std::from_chars(s.data(), s.data() + s.size(), result, base);
        return ConvertResult(r.ec);
    }


    ResultCode TryParseDouble(festd::string_view s, double& result)
    {
        s = s.strip();
        if (s.empty())
            return ResultCode::kEmptyString;

        const std::from_chars_result r = std::from_chars(s.data(), s.data() + s.size(), result);
        return ConvertResult(r.ec);
    }
} // namespace FE::Parser
