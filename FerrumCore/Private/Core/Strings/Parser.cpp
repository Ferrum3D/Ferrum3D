#include <Core/Strings/Parser.h>
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

        const char* end = s.data() + s.size();
        const std::from_chars_result r = std::from_chars(s.data(), end, result, base);
        return r.ptr == end ? ConvertResult(r.ec) : ResultCode::kInvalidFormat;
    }


    ResultCode TryParseInt64(festd::string_view s, int64_t& result, int32_t base)
    {
        s = s.strip();
        if (s.empty())
            return ResultCode::kEmptyString;

        const char* end = s.data() + s.size();
        const std::from_chars_result r = std::from_chars(s.data(), end, result, base);
        return r.ptr == end ? ConvertResult(r.ec) : ResultCode::kInvalidFormat;
    }


    ResultCode TryParseDouble(festd::string_view s, double& result)
    {
        s = s.strip();
        if (s.empty())
            return ResultCode::kEmptyString;

        const char* end = s.data() + s.size();
        const std::from_chars_result r = std::from_chars(s.data(), end, result);
        return r.ptr == end ? ConvertResult(r.ec) : ResultCode::kInvalidFormat;
    }


    ResultCode TryParseHexDouble(festd::string_view s, double& result)
    {
        s = s.strip();
        if (s.empty())
            return ResultCode::kEmptyString;

        bool isNegative = false;
        if (s.byte_at(0) == '-')
        {
            isNegative = true;
            s = festd::string_view{ s.data() + 1, s.size() - 1 };
        }

        if (s.size() < 2 || s.byte_at(0) != '0' || (s.byte_at(1) != 'x' && s.byte_at(1) != 'X'))
            return ResultCode::kInvalidFormat;
        s = festd::string_view{ s.data() + 2, s.size() - 2 };

        const char* end = s.data() + s.size();
        const std::from_chars_result r = std::from_chars(s.data(), end, result, std::chars_format::hex);
        if (r.ptr != end)
            return ResultCode::kInvalidFormat;

        const ResultCode resultCode = ConvertResult(r.ec);
        if (resultCode == ResultCode::kSuccess && isNegative)
            result = -result;
        return resultCode;
    }
} // namespace FE::Parser
