#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/IO/Platform/PlatformPath.h>
#include <FeCore/Platform/Windows/Common.h>

namespace FE::Platform
{
    IO::Path GetCurrentDirectory()
    {
        WCHAR buffer[IO::kMaxPathLength + 1];
        const DWORD pathLength = ::GetCurrentDirectoryW(IO::kMaxPathLength, buffer);
        return ConvertWideString<IO::Path>({ buffer, pathLength });
    }


    void SetCurrentDirectory(const festd::string_view path)
    {
        const WideString widePath{ path };
        ::SetCurrentDirectoryW(widePath.data());
    }


    IO::Path GetExecutablePath()
    {
        DWORD pathLength;
        WCHAR buffer[IO::kMaxPathLength + 1];
        const BOOL result = ::QueryFullProcessImageNameW(::GetCurrentProcess(), 0, buffer, &pathLength);
        FE_CORE_ASSERT(result, "QueryFullProcessImageName failed");
        return ConvertWideString<IO::Path>({ buffer, pathLength });
    }
} // namespace FE::Platform
