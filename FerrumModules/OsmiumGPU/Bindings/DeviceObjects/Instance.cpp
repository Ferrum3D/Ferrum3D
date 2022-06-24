#include <FeCore/Modules/DynamicLibrary.h>
#include <GPU/Adapter/IAdapter.h>
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

        FE_DLL_EXPORT void IInstance_GetAdapters(IInstance* self, IAdapter** adapters, Int32* size)
        {
            auto& a = self->GetAdapters();
            *size   = static_cast<Int32>(a.size());
            if (adapters)
            {
                for (USize i = 0; i < a.size(); ++i)
                {
                    adapters[i] = a[i].Detach();
                }
            }
        }

        FE_DLL_EXPORT void IInstance_Destruct(IInstance* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::GPU
