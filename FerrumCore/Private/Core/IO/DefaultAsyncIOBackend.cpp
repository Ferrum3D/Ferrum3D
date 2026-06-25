#include <Core/IO/AsyncStreamIO.h>
#include <Core/IO/Platform/PlatformFile.h>

namespace FE::IO
{
    AsyncReadHandle DefaultAsyncIOBackend::DispatchRead(const AsyncIOPhysicalRead& read)
    {
        AsyncReadHandle handle{ m_nextHandle++ };

        CompletedRead completion;
        completion.m_handle = handle;
        completion.m_group = read.m_group;

        Platform::FileHandle file;
        ResultCode result = Platform::OpenFile(read.m_filePath, OpenMode::kReadOnly, file);
        if (result == ResultCode::kSuccess)
        {
            result = Platform::SeekFile(file, static_cast<intptr_t>(read.m_offset), SeekMode::kBegin);
            if (result == ResultCode::kSuccess)
                result = Platform::ReadFile(file, read.m_destination, read.m_size, completion.m_bytesRead);

            Platform::CloseFile(file);
        }

        completion.m_result = result;
        m_completions.push_back(completion);
        return handle;
    }


    bool DefaultAsyncIOBackend::PollRequestCompletion(AsyncIOCompletion& completion)
    {
        if (m_completions.empty())
            return false;

        CompletedRead completed = m_completions.back();
        m_completions.pop_back();
        completion.m_handle = completed.m_handle;
        completion.m_group = completed.m_group;
        completion.m_result = completed.m_result;
        completion.m_bytesRead = completed.m_bytesRead;
        return true;
    }


    void DefaultAsyncIOBackend::Cancel(AsyncReadHandle) {}
} // namespace FE::IO
