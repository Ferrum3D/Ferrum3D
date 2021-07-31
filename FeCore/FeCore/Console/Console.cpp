#include "Console.h"
#include <iostream>

#ifdef FE_WINDOWS
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
HANDLE ConHandle = nullptr;
#    undef min
#    undef max
#endif

namespace FE
{
    void FeInitConsole()
    {
#ifdef FE_WINDOWS
        SetConsoleOutputCP(CP_UTF8);
        setvbuf(stdout, nullptr, _IOFBF, 1000);
#endif
    }

    void FeSetConColor(FeConColor color)
    {
#ifdef FE_WINDOWS
        if (!ConHandle)
            ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (ConHandle)
        {
            fflush(stdout);
            SetConsoleTextAttribute(ConHandle, (int)color);
        }
#elif defined(FE_LINUX)
        std::cerr << "\033[" << (int)color << "m";
#endif
    }

    void FeResetConColor()
    {
#ifdef FE_WINDOWS
        if (!ConHandle)
            ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (ConHandle)
            SetConsoleTextAttribute(ConHandle, (int)FeConColor::Def);
#elif defined(FE_LINUX)
        std::cerr << "\033[" << (int)ConColor::Def << "m";
#endif
    }
} // namespace FE
