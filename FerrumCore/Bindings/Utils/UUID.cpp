#include <FeCore/Strings/Format.h>
#include <FeCore/Utils/UUID.h>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT void UUID_FromGUID(const GUID* guid, UUID* result)
        {
            *result = UUID::FromGUID(*guid);
        }

        FE_DLL_EXPORT void UUID_FromString(const char* string, UUID* result)
        {
            *result = UUID(string);
        }

        FE_DLL_EXPORT void UUID_ToString(const UUID* self, char* result)
        {
            auto s = Fmt::Format("{}", *self);
            memcpy(result, s.Data(), s.Size());
        }

        FE_DLL_EXPORT USize UUID_GetHash(const UUID* self)
        {
            return std::hash<UUID>{}(*self);
        }
    }
} // namespace FE
