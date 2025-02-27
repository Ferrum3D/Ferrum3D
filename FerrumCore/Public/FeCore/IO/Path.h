#pragma once
#include <FeCore/Strings/FixedString.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE::IO
{
    inline constexpr uint32_t kMaxPathLength = 260;

    using FixedPath = FixedString<kMaxPathLength>;
} // namespace FE::IO


namespace FE::Path
{
    
}
