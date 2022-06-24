#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>
#include <cstdio>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT void* InitLogger()
        {
            auto logger = MakeShared<Debug::ConsoleLogger>();
            return logger.Detach();
        }

        FE_DLL_EXPORT void DeinitLogger(Debug::IConsoleLogger* logger)
        {
            logger->ReleaseStrongRef();
        }

        FE_DLL_EXPORT void ConsoleLogger_Log(Debug::IConsoleLogger* logger, const char* message, Int32 logType)
        {
            logger->Log(static_cast<Debug::LogMessageType>(logType), "{}", StringSlice(message));
        }
    }
}
