#include <FeCore/IO/Path.h>
#include <FeCore/IO/Platform/PlatformPath.h>

namespace FE::IO
{
    Path GetAbsolutePath(const festd::string_view path)
    {
        return NormalizePath(Directory::GetCurrentDirectory() / path);
    }


    Path NormalizePath(const festd::string_view path)
    {
        Path result;
        if (path.empty())
            return result;

        const char* segmentStart = path.data();
        const char* pathEnd = path.data() + path.size();

        while (segmentStart < pathEnd)
        {
            const char* segmentEnd = PathParser::SkipUntilSeparator(segmentStart, static_cast<uint32_t>(pathEnd - segmentStart));

            if (segmentStart == segmentEnd)
            {
                result.push_back('/');
                ++segmentStart;
                continue;
            }

            const festd::string_view segment{ segmentStart, segmentEnd };
            if (segment == "..")
            {
                if (!result.empty())
                {
                    const auto lastSlashIter = result.find_last_of('/');
                    if (lastSlashIter != result.end())
                        result.resize(static_cast<uint32_t>(lastSlashIter.m_iter - result.data()), 0);
                    else if (result.size() > 0 && result.byte_at(0) == '/')
                        result = "/";
                    else
                        result.clear();
                }
            }
            else if (segment != "." && !segment.empty())
            {
                if (result.empty())
                    result = segment;
                else
                    result /= segment;
            }

            segmentStart = segmentEnd + 1;
        }

        if (result.empty())
        {
            const PathView view{ path };
            if (view.has_root_path())
                return view.root_path();
        }

        return result;
    }


    Path Directory::GetCurrentDirectory()
    {
        return Platform::GetCurrentDirectory();
    }


    void Directory::SetCurrentDirectory(const festd::string_view path)
    {
        Platform::SetCurrentDirectory(path);
    }


    Path Directory::GetExecutableDirectory()
    {
        const Path executablePath = Platform::GetExecutablePath();
        const PathView pathView{ executablePath };
        return pathView.parent_directory();
    }
} // namespace FE::IO
