#include <Core/Base/Base.h>
#include <Core/Base/PlatformInclude.h>
#include <Core/Math/UUID.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace FE
{
    Uuid Uuid::Random()
    {
        FE_PROFILER_ZONE();

        Uuid result{ kForceInit };
        if (BCryptGenRandom(nullptr, result.data(), sizeof(result.m_bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
            return kNull;

        result.m_bytes[6] = (result.m_bytes[6] & 0x0f) | 0x40;
        result.m_bytes[8] = (result.m_bytes[8] & 0x3f) | 0x80;
        return result;
    }
} // namespace FE
