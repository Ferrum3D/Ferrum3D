#pragma once
#include <FeCore/IO/Path.h>

namespace FE::Platform
{
    IO::Path GetCurrentDirectory();
    void SetCurrentDirectory(festd::string_view path);

    IO::Path GetExecutablePath();
} // namespace FE::Platform
