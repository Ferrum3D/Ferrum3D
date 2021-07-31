#pragma once
#include <Utils/CoreUtils.h>
#include <Utils/Platform.h>
#include <mutex>

namespace FE
{
    // clang-format off

    /**
	* @brief Cross-platform abstraction over console text color
	*/
    FE_TYPED_ENUM(FeConColor, uint8_t)
    {
#ifdef FE_WINDOWS
        Red    = 0xC,
        Green  = 0xA,
        Yellow = 0xE,
        Blue   = 0xB,
        White  = 0xF,
        Def    = White
#elif defined(FE_LINUX)
        Red    = 31,
        Green  = 32,
        Yellow = 33,
        Blue   = 34,
        White  = 37,
        Def    = 0
#endif
    };
    // clang-format on

    /**
	* @brief Console mutex, used internally, e.g. in FeLog
	*/
    static std::mutex ConsoleMut;

    /**
	* @brief Initialize console, used internally
	*/
    FE_CORE_API void FeInitConsole();

    /**
	* @brief Set console text color
	*/
    FE_CORE_API void FeSetConColor(FeConColor color);

    /**
	* @brief Reset console text color to default
	*/
    FE_CORE_API void FeResetConColor();
} // namespace FE
