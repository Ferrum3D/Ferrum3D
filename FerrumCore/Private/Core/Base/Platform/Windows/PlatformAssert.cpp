#include <Core/Base/Base.h>
#include <Core/Base/PlatformInclude.h>
#include <Core/Strings/Encoding.h>
#include <Core/Strings/Format.h>

namespace FE
{
    namespace
    {
        struct AssertionReportWriter final
        {
            AssertionReportWriter()
            {
                m_buffer[0] = 0;
            }

            void WriteStr(const char* str, const uint32_t length = Constants::kMaxU32)
            {
                //
                // NOTE: We cannot use WideString here: it can allocate memory if its inline buffer overflows.
                // But the Platform::AssertionReport function might have been called from the memory management code,
                // which means that an allocation here can lead to infinite recursion or memory corruption.
                //

                const uint32_t bufferSize = festd::size(m_buffer);
                const uint32_t destinationSize = bufferSize - m_writePos;
                if (destinationSize <= 1)
                    return;

                char16_t* destination = m_buffer + m_writePos;
                const uint32_t written = Str::ConvertUtf8ToUtf16(str, length, destination, destinationSize);
                if (written != Constants::kMaxU32)
                {
                    // Exclude the null terminator.
                    m_writePos += written - 1;
                }
                else
                {
                    destination[0] = L'?';
                    destination[1] = 0;
                    ++m_writePos;
                }
            }

            void WriteInt(const uint32_t value)
            {
                const Fmt::IntFormatter<uint32_t> fmt{ value };
                WriteStr(fmt.m_buffer, fmt.m_size);
            }

            void WriteLn()
            {
                const uint32_t bufferSize = festd::size(m_buffer);
                const uint32_t destinationSize = bufferSize - m_writePos;
                if (destinationSize <= 1)
                    m_writePos--;

                char16_t* destination = m_buffer + m_writePos;
                destination[0] = L'\n';
                destination[1] = 0;
                ++m_writePos;
            }

            char16_t m_buffer[1024];
            uint32_t m_writePos = 0;
        };
    } // namespace


    void Platform::AssertionReport(const SourceLocation sourceLocation, const char* message, const uint32_t messageSize)
    {
        AssertionReportWriter writer;
        writer.WriteStr(sourceLocation.m_fileName);
        writer.WriteStr("(");
        writer.WriteInt(sourceLocation.m_lineNumber);
        writer.WriteStr("): ");
        writer.WriteStr(message, messageSize);
        writer.WriteLn();

        OutputDebugStringW(reinterpret_cast<LPWSTR>(writer.m_buffer));
    }
} // namespace FE
