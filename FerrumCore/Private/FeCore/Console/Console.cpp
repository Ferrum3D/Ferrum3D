#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Console/Console.h>
#include <FeCore/Modules/EnvironmentPrivate.h>
#include <FeCore/Platform/Windows/Common.h>

namespace FE::Console
{
    namespace
    {
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
            const ConsoleState& state = Env::Internal::SharedState::Get().m_consoleState;
            if (color == Color::kDefault)
            {
                SetConsoleTextAttribute(state.m_consoleHandle, state.m_defaultAttributes);
                return;
            }

            const WORD combinedColor = static_cast<WORD>(color) | (state.m_defaultAttributes & 0xfff0);
            SetConsoleTextAttribute(state.m_consoleHandle, combinedColor);
        }


        struct BufferWriter final
        {
            explicit BufferWriter(ConsoleState& state)
                : m_state(&state)
            {
            }

            void SetTextColor(Color color) const
            {
                FlushIfNecessary(sizeof(BufferRecordHeader));
                const BufferRecordHeader header{ BufferRecordHeaderType::kTextColor, color };
                *m_state->m_bufferPointer = festd::bit_cast<uint8_t>(header);
                m_state->m_bufferPointer += sizeof(BufferRecordHeader);
                m_state->m_lastPayloadSizePointer = nullptr;
            }

            void WriteText(StringSlice text) const
            {
                if (m_state->m_lastPayloadSizePointer != nullptr && GetSpaceLeft() >= text.Size())
                {
                    *m_state->m_lastPayloadSizePointer += text.Size();
                    memcpy(m_state->m_bufferPointer, text.Data(), text.Size());
                    m_state->m_bufferPointer += text.Size();
                    return;
                }

                FlushIfNecessary(sizeof(BufferRecordHeader) + sizeof(uint32_t) + text.Size());
                const BufferRecordHeader header{ BufferRecordHeaderType::kTextPayload, Color::kDefault };
                *m_state->m_bufferPointer = festd::bit_cast<uint8_t>(header);
                m_state->m_bufferPointer += sizeof(BufferRecordHeader);

                m_state->m_lastPayloadSizePointer = reinterpret_cast<uint32_t*>(m_state->m_bufferPointer);
                *m_state->m_lastPayloadSizePointer = text.Size();
                m_state->m_bufferPointer += sizeof(uint32_t);

                memcpy(m_state->m_bufferPointer, text.Data(), text.Size());
                m_state->m_bufferPointer += text.Size();
            }

            void FlushIfNecessary(uint32_t bytes) const
            {
                if (bytes > GetSpaceLeft())
                    Flush();
            }

            void Flush() const
            {
                uint8_t* pointer = m_state->m_buffer;
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

                    const StringSlice payload{ reinterpret_cast<const char*>(pointer), payloadSize };
                    const Platform::WideString<256> wideText{ payload };
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


    void Init()
    {
        ConsoleState& state = Env::Internal::SharedState::Get().m_consoleState;
        state.m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
        state.m_defaultAttributes = info.wAttributes;

        std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
        state.m_buffer = static_cast<uint8_t*>(allocator->allocate(ConsoleState::kBufferSize));
        state.m_bufferPointer = state.m_buffer;

        DWORD consoleProcessIds[2];
        const DWORD processCount = GetConsoleProcessList(consoleProcessIds, 2);
        state.m_isExclusive = processCount <= 1;

        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);

        if (state.m_isExclusive)
        {
            SetConsoleTitleW(L"Ferrum3D Console");
            ShowWindow(GetConsoleWindow(), SW_HIDE);
        }
    }


    void SetTextColor(Color color)
    {
        const BufferWriter writer{ Env::Internal::SharedState::Get().m_consoleState };
        writer.SetTextColor(color);
    }


    void Write(StringSlice text)
    {
        const BufferWriter writer{ Env::Internal::SharedState::Get().m_consoleState };
        writer.WriteText(text);
    }


    void Flush()
    {
        const BufferWriter writer{ Env::Internal::SharedState::Get().m_consoleState };
        writer.Flush();
    }
} // namespace FE::Console
