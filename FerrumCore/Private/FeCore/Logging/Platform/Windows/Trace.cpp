#include <FeCore/Logging/Trace.h>
#include <FeCore/Platform/Windows/Common.h>

using namespace FE::Platform;

namespace FE::Trace::Platform
{
    void AssertionReport(SourceLocation location, StringSlice message)
    {
        const WideString<256> wideMessage{ message };
        const WideString<MAX_PATH> wideFile{ location.m_fileName };
        _wassert(wideMessage.m_value.data(), wideFile.m_value.data(), location.m_lineNumber);
    }
} // namespace FE::Trace::Platform
