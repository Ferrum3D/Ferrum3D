#include <FeCore/DI/BaseDI.h>

namespace FE::DI
{
    const char* GetResultDesc(ResultCode code)
    {
        switch (code)
        {
        case ResultCode::kSuccess:
            return "Success";
        case ResultCode::kAbort:
            return "Operation aborted";
        case ResultCode::kInvalidOperation:
            return "Operation was invalid";
        case ResultCode::kNotFound:
            return "Some of the required services were not found";
        case ResultCode::kCircularDependency:
            return "Service activation failed because of circular dependencies";
        default:
            return "Unknown error";
        }
    }
} // namespace FE::DI
