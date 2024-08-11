#include <FeCore/DI/BaseDI.h>

namespace FE::DI
{
    const char* GetResultDesc(ResultCode code)
    {
        switch (code)
        {
        case ResultCode::Success:
            return "Success";
        case ResultCode::Abort:
            return "Operation aborted";
        case ResultCode::InvalidOperation:
            return "Operation was invalid";
        case ResultCode::NotFound:
            return "Some of the required services were not found";
        case ResultCode::CircularDependency:
            return "Service activation failed because of circular dependencies";
        default:
            return "Unknown error";
        }
    }
} // namespace FE::DI
