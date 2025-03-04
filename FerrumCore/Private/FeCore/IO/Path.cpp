#include <FeCore/IO/Path.h>
#include <FeCore/IO/Platform/PlatformPath.h>

namespace FE::IO::Directory
{
    Path GetCurrentDirectory()
    {
        return Platform::GetCurrentDirectory();
    }


    void SetCurrentDirectory(const festd::string_view path)
    {
        Platform::SetCurrentDirectory(path);
    }


    Path GetExecutableDirectory()
    {
        const Path executablePath = Platform::GetExecutablePath();
        const PathView pathView{ executablePath };
        return pathView.parent_directory();
    }


    Path Normalize(const festd::string_view path)
    {
        Path result;
        for (const int32_t codepoint : path)
        {
            if (codepoint == '\\')
            {
                result.push_back('/');
                continue;
            }

            result.append(codepoint);
        }

        return result;
    }
} // namespace FE::IO::Directory
