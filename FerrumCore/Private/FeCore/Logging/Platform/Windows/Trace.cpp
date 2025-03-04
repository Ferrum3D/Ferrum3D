#include <FeCore/Logging/Trace.h>
#include <FeCore/Platform/Windows/Common.h>

using namespace FE::Platform;

namespace FE::Trace::Platform
{
    void AssertionReport(const SourceLocation location, const festd::string_view message)
    {
        const WideString wideMessage{ message };
        const WideString wideFile{ location.m_fileName };
        _wassert(wideMessage.m_value.data(), wideFile.m_value.data(), location.m_lineNumber);
    }
} // namespace FE::Trace::Platform
