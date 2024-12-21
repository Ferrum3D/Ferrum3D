#pragma once
#include <FeCore/Console/Console.h>

namespace FE::Console
{
    struct ConsoleState final
    {
        static constexpr uint32_t kBufferSize = 4096;

        void* m_consoleHandle = nullptr;
        uint16_t m_defaultAttributes = 0;
        bool m_isExclusive = false;

        uint8_t* m_buffer = nullptr;
        uint8_t* m_bufferPointer = nullptr;
        uint32_t* m_lastPayloadSizePointer = nullptr;
    };
} // namespace FE::Console
