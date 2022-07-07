#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>

namespace FE::Debug
{
    extern "C"
    {
        FE_DLL_EXPORT IConsoleLogger* IConsoleLogger_Construct()
        {
            auto logger = MakeShared<ConsoleLogger>();
            return logger.Detach();
        }

        FE_DLL_EXPORT void IConsoleLogger_Destruct(IConsoleLogger* self)
        {
            self->ReleaseStrongRef();
        }

        FE_DLL_EXPORT void IConsoleLogger_Log(IConsoleLogger* self, const char* message, Int32 logType)
        {
            self->Log(static_cast<LogMessageType>(logType), "{}", StringSlice(message));
        }
    }
}
