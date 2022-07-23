#include <Bindings/ECS/ComponentType.h>
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/ECS/EntityQuery.h>

namespace FE::ECS
{
    extern "C"
    {
        FE_DLL_EXPORT EntityQuery* EntityQuery_Construct(EntityRegistry* registry)
        {
            return MakeShared<EntityQuery>(registry).Detach();
        }

        FE_DLL_EXPORT void EntityQuery_NoneOf(EntityQuery* self, ComponentTypeBinding* componentTypes, UInt32 componentTypeCount)
        {
            List<ComponentType> types;
            types.Reserve(componentTypeCount);
            for (USize i = 0; i < componentTypeCount; ++i)
            {
                types.Push(componentTypes[i].Convert());
            }

            self->NoneOf(types);
        }

        FE_DLL_EXPORT void EntityQuery_AllOf(EntityQuery* self, ComponentTypeBinding* componentTypes, UInt32 componentTypeCount)
        {
            List<ComponentType> types;
            types.Reserve(componentTypeCount);
            for (USize i = 0; i < componentTypeCount; ++i)
            {
                types.Push(componentTypes[i].Convert());
            }

            self->AllOf(types);
        }

        FE_DLL_EXPORT void EntityQuery_AnyOf(EntityQuery* self, ComponentTypeBinding* componentTypes, UInt32 componentTypeCount)
        {
            List<ComponentType> types;
            types.Reserve(componentTypeCount);
            for (USize i = 0; i < componentTypeCount; ++i)
            {
                types.Push(componentTypes[i].Convert());
            }

            self->AnyOf(types);
        }

        FE_DLL_EXPORT void EntityQuery_Update(EntityQuery* self)
        {
            self->Update();
        }

        FE_DLL_EXPORT void EntityQuery_GetChunks(EntityQuery* self, ByteBuffer* result)
        {
            *result = ByteBuffer::MoveList(self->GetChunks());
        }

        FE_DLL_EXPORT void EntityQuery_Destruct(EntityQuery* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::ECS
