#include <FeCore/Modules/DynamicLibrary.h>

namespace FE::Osmium
{
    Shared<DynamicLibrary> g_OsmiumLib;

    extern "C"
    {
        FE_DLL_EXPORT void AttachEnvironment(Env::Internal::IEnvironment* env)
        {
            Env::AttachEnvironment(*env);
            g_OsmiumLib = MakeShared<DynamicLibrary>("OsmiumGPU");
        }

        FE_DLL_EXPORT void DetachEnvironment()
        {
            g_OsmiumLib.Reset();
        }
    }
} // namespace FE::Osmium
