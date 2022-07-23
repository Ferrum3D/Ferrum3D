#include <FeCore/ECS/World.h>
#include <FeCore/Systems/TransformSystem.h>

namespace FE::ECS
{
    extern "C"
    {
        FE_DLL_EXPORT IWorld* World_Construct()
        {
            return MakeShared<World>().Detach();
        }

        FE_DLL_EXPORT void World_RegisterCoreSystems(IWorld* self)
        {
            self->RegisterSystem(MakeShared<TransformSystem>().Detach());
        }

        FE_DLL_EXPORT void World_RegisterSystem(IWorld* self, ISystem* system)
        {
            self->RegisterSystem(system);
        }

        FE_DLL_EXPORT void World_UnregisterSystem(IWorld* self, ISystem* system)
        {
            self->UnregisterSystem(system);
        }

        FE_DLL_EXPORT EntityRegistry* World_Registry(IWorld* self)
        {
            return self->Registry();
        }

        FE_DLL_EXPORT void World_Destruct(IWorld* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::ECS
