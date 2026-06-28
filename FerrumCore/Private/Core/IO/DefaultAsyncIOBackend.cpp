#include <Core/IO/AsyncStreamIO.h>
#include <Core/IO/DefaultAsyncIOBackend.h>
#include <Core/IO/Platform/PlatformFile.h>

namespace FE::IO
{
    ResultCode DefaultAsyncIOBackend::OpenFile(const festd::string_view filePath, Platform::FileHandle& fileHandle)
    {
        return Platform::OpenFile(filePath, OpenMode::kReadOnly, fileHandle);
    }


    AsyncReadHandle DefaultAsyncIOBackend::DispatchRead(const AsyncIOPhysicalRead& read)
    {
        const AsyncReadHandle handle{ m_completions.size() };

        AsyncIOCompletion completion;
        completion.m_handle = handle;
        completion.m_group = read.m_group;

        ResultCode result = Platform::SeekFile(read.m_fileHandle, static_cast<intptr_t>(read.m_offset), SeekMode::kBegin);
        if (result == ResultCode::kSuccess)
            result = Platform::ReadFile(read.m_fileHandle, read.m_destination, read.m_size, completion.m_bytesRead);

        completion.m_result = result;
        m_completions.push_back(completion);
        return handle;
    }


    bool DefaultAsyncIOBackend::PollRequestCompletion(AsyncIOCompletion& completion)
    {
        if (m_completions.empty())
            return false;

        completion = m_completions.back();
        m_completions.pop_back();
        return true;
    }
} // namespace FE::IO
