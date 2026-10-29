#pragma once
#include <Core/IO/Path.h>
#include <festd/vector.h>

namespace FE::AssetBuilder
{
    //! @brief Broadcasts text lines to every client connected to a named pipe.
    //!
    //! Writes never block for long: a client that does not read its messages in time is disconnected. Not thread-safe.
    struct StatusPipe final
    {
        StatusPipe() = default;
        ~StatusPipe();

        StatusPipe(const StatusPipe&) = delete;
        StatusPipe& operator=(const StatusPipe&) = delete;

        //! @brief Create the pipe. Fails if another process already serves a pipe with the same name.
        bool Open(festd::string_view name);

        //! @brief Accept clients that connected since the previous call.
        void AcceptClients();

        //! @brief Send one line to every connected client.
        void Broadcast(festd::string_view line);

        [[nodiscard]] uint32_t GetClientCount() const
        {
            return m_clients.size();
        }

    private:
        bool CreateListeningInstance(bool isFirstInstance);
        void CloseClient(uint32_t index);

        festd::vector<wchar_t> m_name;
        void* m_listeningPipe = nullptr;
        void* m_connectEvent = nullptr;

        //! Storage for the OVERLAPPED of the pending connection, which must stay alive until the connection completes.
        alignas(8) std::byte m_connectOverlapped[32] = {};

        void* m_writeEvent = nullptr;
        bool m_isConnecting = false;
        festd::vector<void*> m_clients;
    };
} // namespace FE::AssetBuilder
