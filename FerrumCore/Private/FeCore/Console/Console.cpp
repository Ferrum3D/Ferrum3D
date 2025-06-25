#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Console/Console.h>
#include <FeCore/Console/ConsolePrivate.h>
#include <FeCore/Platform/Windows/Common.h>

namespace FE::Console
{
    namespace
    {
        struct ConsoleState final
        {
            static constexpr uint32_t kBufferSize = 4096;

            void* m_consoleHandle = nullptr;
            uint16_t m_defaultAttributes = 0;
            bool m_isExclusive = false;

            std::byte m_buffer[kBufferSize];
            std::byte* m_bufferPointer = nullptr;
            uint32_t* m_lastPayloadSizePointer = nullptr;
        };

        ConsoleState* GConsoleState;


        enum class BufferRecordHeaderType : uint8_t
        {
            kTextColor = 0,
            kTextPayload = 1,
        };


        struct BufferRecordHeader final
        {
            BufferRecordHeaderType m_headerType : 1;
            Color m_color : 7;
        };

        static_assert(sizeof(BufferRecordHeader) == 1);


        void SetTextColorImpl(Color color)
        {
            if (color == Color::kDefault)
            {
                SetConsoleTextAttribute(GConsoleState->m_consoleHandle, GConsoleState->m_defaultAttributes);
                return;
            }

            const WORD combinedColor = static_cast<WORD>(color) | (GConsoleState->m_defaultAttributes & 0xfff0);
            SetConsoleTextAttribute(GConsoleState->m_consoleHandle, combinedColor);
        }


        struct BufferWriter final
        {
            BufferWriter()
                : m_state(GConsoleState)
            {
            }

            void SetTextColor(const Color color) const
            {
                FlushIfNecessary(sizeof(BufferRecordHeader));
                const BufferRecordHeader header{ BufferRecordHeaderType::kTextColor, color };
                *m_state->m_bufferPointer = festd::bit_cast<std::byte>(header);
                m_state->m_bufferPointer += sizeof(BufferRecordHeader);
                m_state->m_lastPayloadSizePointer = nullptr;
            }

            void WriteText(const festd::string_view text) const
            {
                if (m_state->m_lastPayloadSizePointer != nullptr && GetSpaceLeft() >= text.size())
                {
                    *m_state->m_lastPayloadSizePointer += text.size();
                    memcpy(m_state->m_bufferPointer, text.data(), text.size());
                    m_state->m_bufferPointer += text.size();
                    return;
                }

                FlushIfNecessary(sizeof(BufferRecordHeader) + sizeof(uint32_t) + text.size());
                const BufferRecordHeader header{ BufferRecordHeaderType::kTextPayload, Color::kDefault };
                *m_state->m_bufferPointer = festd::bit_cast<std::byte>(header);
                m_state->m_bufferPointer += sizeof(BufferRecordHeader);

                m_state->m_lastPayloadSizePointer = reinterpret_cast<uint32_t*>(m_state->m_bufferPointer);
                *m_state->m_lastPayloadSizePointer = text.size();
                m_state->m_bufferPointer += sizeof(uint32_t);

                memcpy(m_state->m_bufferPointer, text.data(), text.size());
                m_state->m_bufferPointer += text.size();
            }

            void FlushIfNecessary(const uint32_t bytes) const
            {
                if (bytes > GetSpaceLeft())
                    Flush();
            }

            void Flush() const
            {
                std::byte* pointer = m_state->m_buffer;
                while (pointer < m_state->m_bufferPointer)
                {
                    const BufferRecordHeader header = festd::bit_cast<BufferRecordHeader>(*pointer);
                    ++pointer;
                    if (header.m_headerType == BufferRecordHeaderType::kTextColor)
                    {
                        SetTextColorImpl(header.m_color);
                        continue;
                    }

                    const uint32_t payloadSize = *reinterpret_cast<uint32_t*>(pointer);
                    pointer += sizeof(uint32_t);

                    const festd::string_view payload{ reinterpret_cast<const char*>(pointer), payloadSize };
                    const Platform::WideString wideText{ payload };
                    WriteConsoleW(m_state->m_consoleHandle, wideText.data(), wideText.size(), nullptr, nullptr);

                    if (IsDebuggerPresent())
                    {
                        OutputDebugStringW(wideText.data());
                    }

                    pointer += payloadSize;
                }

                m_state->m_bufferPointer = m_state->m_buffer;
                m_state->m_lastPayloadSizePointer = nullptr;
            }

            [[nodiscard]] uint32_t GetSpaceLeft() const
            {
                return ConsoleState::kBufferSize - static_cast<uint32_t>(m_state->m_bufferPointer - m_state->m_buffer);
            }

            ConsoleState* m_state;
        };
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_CoreAssert(GConsoleState == nullptr, "Console already initialized");
        GConsoleState = Memory::New<ConsoleState>(allocator);

        GConsoleState->m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(GConsoleState->m_consoleHandle, &info);
        GConsoleState->m_defaultAttributes = info.wAttributes;
        GConsoleState->m_bufferPointer = GConsoleState->m_buffer;

        DWORD consoleProcessIds[2];
        const DWORD processCount = GetConsoleProcessList(consoleProcessIds, 2);
        GConsoleState->m_isExclusive = processCount <= 1;

        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);

        if (GConsoleState->m_isExclusive)
        {
            SetConsoleTitleW(L"Ferrum3D Console");
            ShowWindow(GetConsoleWindow(), SW_HIDE);
        }
    }


    void Internal::Shutdown()
    {
        FE_CoreAssert(GConsoleState != nullptr, "Console not initialized");
        GConsoleState->~ConsoleState();
        GConsoleState = nullptr;
    }


    void SetTextColor(const Color color)
    {
        const BufferWriter writer;
        writer.SetTextColor(color);
    }


    void Write(const festd::string_view text)
    {
        const BufferWriter writer;
        writer.WriteText(text);
    }


    void Flush()
    {
        const BufferWriter writer;
        writer.Flush();
    }
} // namespace FE::Console
