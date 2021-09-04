#include <FeCore/IO/StdoutStream.h>

namespace FE::IO
{
    bool StdoutStream::IsOpen() const
    {
        return true;
    }

    USize StdoutStream::WriteFromBuffer(const void* buffer, USize size)
    {
        FE_ASSERT_MSG(buffer, "Buffer was nullptr");
        UniqueLocker lk(FE::Console::StdoutMutex);
        FE::Console::PrintToStdout(StringSlice(static_cast<const char*>(buffer), size));
        return 0;
    }

    StringSlice StdoutStream::GetName()
    {
        return "stdout";
    }

    void StdoutStream::Close() {}
} // namespace FE::IO
