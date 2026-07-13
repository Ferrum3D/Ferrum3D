#pragma once
#include <Core/IO/AsyncImpl.h>

namespace FE::IO::Async
{
    struct DefaultAsyncIOBackend final : public IAsyncIOBackend
    {
        FE_RTTI("C1752D59-0343-46D0-B95A-127EB2321CC7");

        festd::expected<Platform::FileHandle, ResultCode> OpenFile(festd::string_view filePath) override;
        AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) override;
        bool PollRequestCompletion(AsyncIOCompletion& completion) override;

    private:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        festd::vector<AsyncIOCompletion> m_completions;
    };
} // namespace FE::IO::Async
