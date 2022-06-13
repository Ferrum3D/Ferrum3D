#include <GPU/Instance/IInstance.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void AttachEnvironment(Env::Internal::IEnvironment* env)
        {
            Env::AttachEnvironment(*env);
        }

        FE_DLL_EXPORT void DetachEnvironment()
        {
            Env::DetachEnvironment();
        }

        FE_DLL_EXPORT IInstance* IInstance_Construct(InstanceDesc* desc, Int32 api)
        {
            auto instance = CreateGraphicsAPIInstance(*desc, static_cast<GraphicsAPI>(api));
            return instance.Detach();
        }

        FE_DLL_EXPORT void IInstance_Destruct(IInstance* instance)
        {
            instance->ReleaseStrongRef();
        }
    }
}
