#pragma once
#include <festd/string.h>

namespace FE::Graphics::Core
{
    //! @brief Make shader sources in an absolute directory visible to shader compilation.
    //!
    //! Shaders and includes are requested by virtual paths, e.g. "Shaders/Passes/Tools/Blit/Blit.ps.hlsl". A file at
    //! <directory>/<relative path> is visible as <virtualPrefix><relative path>. Registering the same pair again has no effect.
    //! Modules register their directories during initialization, before creating pipelines that use them.
    void AddShaderSourceDirectory(festd::string_view directory, festd::string_view virtualPrefix = {});
} // namespace FE::Graphics::Core
