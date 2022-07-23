#include <FeCore/Systems/CallbackSystem.h>

namespace FE::ECS
{
    extern "C"
    {
        FE_DLL_EXPORT ISystem* CallbackSystem_Construct()
        {
            return MakeShared<CallbackSystem>().Detach();
        }

        FE_DLL_EXPORT void CallbackSystem_SetCreateCallback(ISystem* self, CallbackSystem::CreateProc callback)
        {
            fe_assert_cast<CallbackSystem*>(self)->CreateCallback = callback;
        }

        FE_DLL_EXPORT void CallbackSystem_SetUpdateCallback(ISystem* self, CallbackSystem::UpdateProc callback)
        {
            fe_assert_cast<CallbackSystem*>(self)->UpdateCallback = callback;
        }

        FE_DLL_EXPORT void CallbackSystem_SetDestroyCallback(ISystem* self, CallbackSystem::DestroyProc callback)
        {
            fe_assert_cast<CallbackSystem*>(self)->DestroyCallback = callback;
        }

        FE_DLL_EXPORT void CallbackSystem_Destruct(ISystem* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::ECS
