#pragma once
#include <Core/IO/AsyncStreamIO.h>

namespace FE::IO
{
    struct DefaultAsyncIOBackend final : public IAsyncIOBackend
    {
        FE_RTTI("C1752D59-0343-46D0-B95A-127EB2321CC7");

        ResultCode OpenFile(festd::string_view filePath, Platform::FileHandle& fileHandle) override;
        AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) override;
        bool PollRequestCompletion(AsyncIOCompletion& completion) override;

    private:
        festd::vector<AsyncIOCompletion> m_completions;
    };
} // namespace FE::IO
