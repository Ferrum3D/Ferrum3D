#pragma once
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Base/Base.h>
#include <FeCore/Base/Platform.h>
#include <FeCore/Parallel/Locker.h>

namespace FE::Console
{
    //! \brief Cross-platform abstraction over console text color.
    enum class Color
    {
        Default = 0xff,
#ifdef FE_WINDOWS
        Black   = 0x0,
        Navy    = 0x1,
        Green   = 0x2,
        Teal    = 0x3,
        Maroon  = 0x4,
        Purple  = 0x5,
        Olive   = 0x6,
        Silver  = 0x7,
        Gray    = 0x8,
        Blue    = 0x9,
        Lime    = 0xa,
        Aqua    = 0xb,
        Red     = 0xc,
        Fuchsia = 0xd,
        Yellow  = 0xe,
        White   = 0xf
#else
        Black   = 0,
        Navy    = 4,
        Green   = 2,
        Teal    = 6,
        Maroon  = 1,
        Purple  = 5,
        Olive   = 3,
        Silver  = 7,
        Gray    = 8,
        Blue    = 12,
        Lime    = 10,
        Aqua    = 14,
        Red     = 9,
        Fuchsia = 13,
        Yellow  = 11,
        White   = 15
#endif
    };

    static Mutex StdoutMutex;

    //! \brief Initialize the console.
    void Init();

    //! \brief Set console text color.
    //!
    //! \param [in] color - The color to set.
    void SetColor(Color color);

    //! \brief Reset console text color to default.
    void ResetColor();

    //! \brief Print a string to stdout.
    //!
    //! \param [in] string - String to print.
    void PrintToStdout(StringSlice string);
} // namespace FE::Console
