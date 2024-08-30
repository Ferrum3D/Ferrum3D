#include <FeCore/Logging/Trace.h>
#include <FeCore/Platform/Windows/Common.h>

using namespace FE::Platform;

namespace FE::Trace::Platform
{
    void AssertionReport(SourceLocation location, StringSlice message)
    {
        const WideString<256> wideMessage{ message };
        const WideString<MAX_PATH> wideFile{ location.FileName };
        _wassert(wideMessage.Value.data(), wideFile.Value.data(), location.LineNumber);
    }
} // namespace FE::Trace::Platform
