#include "FeCore/Modules/DynamicLibrary.h"
#include <GPU/Instance/IInstance.h>

namespace FE::GPU
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

        FE_DLL_EXPORT IInstance* IInstance_Construct(InstanceDesc* desc, Int32 api)
        {
            auto attachEnvironment = g_OsmiumLib->GetFunction<AttachEnvironmentProc>("AttachEnvironment");
            attachEnvironment(&FE::Env::GetEnvironment());
            auto createGraphicsAPIInstance = g_OsmiumLib->GetFunction<CreateGraphicsAPIInstanceProc>("CreateGraphicsAPIInstance");

            return createGraphicsAPIInstance(*desc, static_cast<GraphicsAPI>(api));
        }

        FE_DLL_EXPORT void IInstance_Destruct(IInstance* instance)
        {
            instance->ReleaseStrongRef();
        }
    }
}
