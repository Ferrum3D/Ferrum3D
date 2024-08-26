#include <FeCore/IO/StdoutStream.h>

namespace FE::IO
{
    bool StdoutStream::IsOpen() const
    {
        return true;
    }

    size_t StdoutStream::WriteFromBuffer(const void* buffer, size_t size)
    {
        FE_ASSERT_MSG(buffer, "Buffer was nullptr");
        const std::unique_lock lk(FE::Console::StdoutMutex);
        FE::Console::PrintToStdout(StringSlice(static_cast<const char*>(buffer), static_cast<uint32_t>(size)));
        return 0;
    }

    StringSlice StdoutStream::GetName()
    {
        return "stdout";
    }

    void StdoutStream::Close() {}
} // namespace FE::IO
