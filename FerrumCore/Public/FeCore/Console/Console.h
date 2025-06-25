#pragma once
#include <festd/string.h>

namespace FE::Console
{
    //! @brief Cross-platform abstraction over console text color.
    enum class Color : uint8_t
    {
        kDefault = 0x7f,
#ifdef FE_PLATFORM_WINDOWS
        kBlack = 0x0,
        kNavy = 0x1,
        kGreen = 0x2,
        kTeal = 0x3,
        kMaroon = 0x4,
        kPurple = 0x5,
        kOlive = 0x6,
        kSilver = 0x7,
        kGray = 0x8,
        kBlue = 0x9,
        kLime = 0xa,
        kAqua = 0xb,
        kRed = 0xc,
        kFuchsia = 0xd,
        kYellow = 0xe,
        kWhite = 0xf
#else
#    error Unsupported Platform
#endif
    };


    void SetTextColor(Color color);
    void Write(festd::string_view text);
    void Flush();
} // namespace FE::Console
