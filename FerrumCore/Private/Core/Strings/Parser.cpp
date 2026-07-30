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

        if (base == 16 && s.starts_with("0x"))
            s = s.substr_ascii(2);

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
} // namespace FE::Parser
