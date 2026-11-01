#include <Core/Base/Base.h>
#include <Core/Base/PlatformInclude.h>
#include <Core/Strings/Encoding.h>
#include <Core/Strings/Format.h>
#include <crtdbg.h>
#include <stdlib.h>

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


        //! Failed assertions break into an attached debugger. Without one, they and other crashes terminate the process
        //! instead of opening a dialog that waits for the user, and CRT reports go to stderr and the debugger output.
        struct ErrorDialogSuppressor final
        {
            ErrorDialogSuppressor()
            {
                SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
                _set_error_mode(_OUT_TO_STDERR);
                _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
                for ([[maybe_unused]] const int reportType : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT })
                {
                    _CrtSetReportMode(reportType, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
                    _CrtSetReportFile(reportType, _CRTDBG_FILE_STDERR);
                }
            }
        };

        // Constructed before main, so that assertions during static initialization do not open dialogs either.
        const ErrorDialogSuppressor GErrorDialogSuppressor;
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

        // Without dialogs, stderr keeps the report visible in console runs and their logs.
        const HANDLE errorOutput = GetStdHandle(STD_ERROR_HANDLE);
        if (errorOutput == nullptr || errorOutput == INVALID_HANDLE_VALUE)
            return;

        const auto writeError = [errorOutput](const char* text, const uint32_t length) {
            DWORD written;
            WriteFile(errorOutput, text, length, &written, nullptr);
        };

        const Fmt::IntFormatter<uint32_t> line{ sourceLocation.m_lineNumber };
        writeError(sourceLocation.m_fileName, static_cast<uint32_t>(__builtin_strlen(sourceLocation.m_fileName)));
        writeError("(", 1);
        writeError(line.m_buffer, line.m_size);
        writeError("): ", 3);
        writeError(message, messageSize);
        writeError("\n", 1);
    }
} // namespace FE
