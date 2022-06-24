#include <FeCore/Time/DateTime.h>

using namespace FE;

inline constexpr size_t g_FormatTimeBufferSize = 128;

void DateTime::Format(std::ostream& stream, const char* format) const
{
    char buffer[g_FormatTimeBufferSize];
    strftime(buffer, g_FormatTimeBufferSize, format, &m_Data);
    stream << buffer;
}

String FE::DateTime::ToString(const char* format) const
{
    char buffer[g_FormatTimeBufferSize];
    strftime(buffer, g_FormatTimeBufferSize, format, &m_Data);
    return buffer;
}
