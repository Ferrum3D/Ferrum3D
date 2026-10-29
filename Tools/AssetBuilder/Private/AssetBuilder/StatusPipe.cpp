#include <AssetBuilder/AssetServer.h>
#include <AssetBuilder/StatusPipe.h>
#include <Core/Base/PlatformInclude.h>
#include <Core/Strings/Encoding.h>

namespace FE::AssetBuilder
{
    namespace
    {
        constexpr DWORD kPipeBufferSize = 64 * 1024;

        //! Time a client gets to accept a message before it is disconnected.
        constexpr DWORD kWriteTimeoutMilliseconds = 100;
    } // namespace


    static_assert(sizeof(OVERLAPPED) <= 32 && alignof(OVERLAPPED) <= 8);


    StatusPipe::~StatusPipe()
    {
        for (uint32_t index = m_clients.size(); index > 0; --index)
            CloseClient(index - 1);

        if (m_listeningPipe != nullptr)
        {
            CancelIo(m_listeningPipe);
            CloseHandle(m_listeningPipe);
        }

        if (m_connectEvent != nullptr)
            CloseHandle(m_connectEvent);

        if (m_writeEvent != nullptr)
            CloseHandle(m_writeEvent);
    }


    bool StatusPipe::Open(const festd::string_view name)
    {
        const Str::Utf8ToUtf16 wideName{ name.data(), name.size() };
        m_name.assign(wideName.data(), wideName.data() + wideName.size());
        m_name.push_back(L'\0');

        m_connectEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        m_writeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (m_connectEvent == nullptr || m_writeEvent == nullptr)
            return false;

        return CreateListeningInstance(true);
    }


    void StatusPipe::AcceptClients()
    {
        while (m_listeningPipe != nullptr)
        {
            if (m_isConnecting)
            {
                if (WaitForSingleObject(m_connectEvent, 0) != WAIT_OBJECT_0)
                    return;

                DWORD bytesTransferred = 0;
                auto* overlapped = reinterpret_cast<OVERLAPPED*>(m_connectOverlapped);
                GetOverlappedResult(m_listeningPipe, overlapped, &bytesTransferred, FALSE);
            }

            m_clients.push_back(m_listeningPipe);
            m_listeningPipe = nullptr;
            CreateListeningInstance(false);
        }
    }


    void StatusPipe::Broadcast(const festd::string_view line)
    {
        AcceptClients();

        festd::string message{ line };
        message.push_back('\n');

        for (uint32_t index = m_clients.size(); index > 0; --index)
        {
            const HANDLE client = m_clients[index - 1];

            OVERLAPPED overlapped{};
            overlapped.hEvent = m_writeEvent;
            ResetEvent(m_writeEvent);

            DWORD bytesWritten = 0;
            bool isWritten = WriteFile(client, message.data(), message.size(), &bytesWritten, &overlapped);
            if (!isWritten && GetLastError() == ERROR_IO_PENDING)
            {
                if (WaitForSingleObject(m_writeEvent, kWriteTimeoutMilliseconds) == WAIT_OBJECT_0)
                {
                    isWritten = GetOverlappedResult(client, &overlapped, &bytesWritten, FALSE);
                }
                else
                {
                    CancelIoEx(client, &overlapped);
                    GetOverlappedResult(client, &overlapped, &bytesWritten, TRUE);
                }
            }

            if (!isWritten)
                CloseClient(index - 1);
        }
    }


    bool StatusPipe::CreateListeningInstance(const bool isFirstInstance)
    {
        DWORD openMode = PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED;
        if (isFirstInstance)
            openMode |= FILE_FLAG_FIRST_PIPE_INSTANCE;

        const HANDLE pipe = CreateNamedPipeW(m_name.data(),
                                             openMode,
                                             PIPE_TYPE_BYTE | PIPE_REJECT_REMOTE_CLIENTS,
                                             PIPE_UNLIMITED_INSTANCES,
                                             kPipeBufferSize,
                                             0,
                                             0,
                                             nullptr);
        if (pipe == INVALID_HANDLE_VALUE)
            return false;

        m_listeningPipe = pipe;
        ResetEvent(m_connectEvent);

        auto* overlapped = reinterpret_cast<OVERLAPPED*>(m_connectOverlapped);
        *overlapped = OVERLAPPED{};
        overlapped->hEvent = m_connectEvent;
        const bool isConnected = ConnectNamedPipe(pipe, overlapped);
        const DWORD error = isConnected ? ERROR_SUCCESS : GetLastError();

        // A client that connected between creation and ConnectNamedPipe is reported as ERROR_PIPE_CONNECTED.
        m_isConnecting = error == ERROR_IO_PENDING;
        if (!m_isConnecting && error != ERROR_PIPE_CONNECTED && error != ERROR_SUCCESS)
        {
            CloseHandle(pipe);
            m_listeningPipe = nullptr;
            return false;
        }

        if (!m_isConnecting)
            SetEvent(m_connectEvent);

        return true;
    }


    void StatusPipe::CloseClient(const uint32_t index)
    {
        const HANDLE client = m_clients[index];
        DisconnectNamedPipe(client);
        CloseHandle(client);
        m_clients.erase(m_clients.begin() + index);
    }


    festd::string GetAssetServerPipeName(const IO::Path& outputDirectory)
    {
        const IO::Path normalizedOutput = IO::NormalizePath(IO::GetAbsolutePath(outputDirectory));
        const uint64_t key = HashPathKey(festd::string_view{ normalizedOutput.data(), normalizedOutput.size() });

        festd::string name{ "\\\\.\\pipe\\ferrum-asset-server-" };
        constexpr const char* kDigits = "0123456789abcdef";
        for (uint32_t index = 0; index < 16; ++index)
            name.push_back(kDigits[(key >> (60 - index * 4)) & 0xf]);

        return name;
    }
} // namespace FE::AssetBuilder
