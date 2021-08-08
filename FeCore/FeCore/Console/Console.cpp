#include <FeCore/Console/Console.h>
#include <iostream>

#ifdef FE_WINDOWS
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#    undef min
#    undef max

static HANDLE ConHandle  = nullptr;
static WORD DefaultColor = 0;

inline static WORD GetConColor()
{
    CONSOLE_SCREEN_BUFFER_INFO info{};
    GetConsoleScreenBufferInfo(ConHandle, &info);
    return info.wAttributes;
}
#endif

namespace FE::Console
{
    void Init()
    {
#ifdef FE_WINDOWS
        ConHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleOutputCP(CP_UTF8);
        DefaultColor = GetConColor();
#endif
    }

    void SetColor(Color color)
    {
#ifdef FE_WINDOWS
        if (ConHandle)
        {
            if (color == Color::Default)
            {
                SetConsoleTextAttribute(ConHandle, DefaultColor);
            }
            else
            {
                SetConsoleTextAttribute(ConHandle, (DefaultColor & 0xF0) | (WORD)color);
            }
        }
#elif defined(FE_LINUX)
        std::cout << "\033[" << (int)color << "m";
#endif
    }

    void ResetColor()
    {
        SetColor(Color::Default);
    }

    void PrintToStdout(StringSlice string)
    {
        std::cout << string;
    }
} // namespace FE::Console
