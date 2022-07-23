#include <Bindings/ECS/ComponentType.h>
#include <FeCore/ECS/EntityRegistry.h>

namespace FE::ECS
{
    extern "C"
    {
        FE_DLL_EXPORT EntityRegistry* EntityRegistry_Construct()
        {
            return MakeShared<EntityRegistry>().Detach();
        }

        FE_DLL_EXPORT void EntityRegistry_Destruct(EntityRegistry* self)
        {
            self->ReleaseStrongRef();
        }

        FE_DLL_EXPORT EntityID EntityRegistry_GetCurrentEntityVersion(EntityRegistry* self, Entity* entity)
        {
            return self->GetCurrentEntityVersion(*entity);
        }

        FE_DLL_EXPORT bool EntityRegistry_IsValid(EntityRegistry* self, Entity* entity)
        {
            return self->IsValid(*entity);
        }

        FE_DLL_EXPORT bool EntityRegistry_AddComponent(EntityRegistry* self, Entity* entity, ComponentTypeBinding* componentType)
        {
            return self->AddComponent(*entity, componentType->Convert());
        }

        FE_DLL_EXPORT void EntityRegistry_AddComponentToEntities(EntityRegistry* self, Entity* entities, UInt32 entityCount,
                                                                 ComponentTypeBinding* componentType)
        {
            return self->AddComponent(ArraySlice(entities, entities + entityCount), componentType->Convert());
        }

        FE_DLL_EXPORT bool EntityRegistry_RemoveComponent(EntityRegistry* self, Entity* entity,
                                                          ComponentTypeBinding* componentType)
        {
            return self->RemoveComponent(*entity, componentType->Convert());
        }

        FE_DLL_EXPORT void EntityRegistry_RemoveComponentFromEntities(EntityRegistry* self, Entity* entities, UInt32 entityCount,
                                                                      ComponentTypeBinding* componentType)
        {
            return self->RemoveComponent(ArraySlice(entities, entities + entityCount), componentType->Convert());
        }

        FE_DLL_EXPORT void EntityRegistry_CreateEntity(EntityRegistry* self, Entity* result)
        {
            *result = self->CreateEntity();
        }

        FE_DLL_EXPORT void EntityRegistry_CreateEntityWithComponents(EntityRegistry* self, ComponentTypeBinding* componentTypes,
                                                                     UInt32 componentTypeCount, Entity* result)
        {
            List<ComponentType> types;
            types.Reserve(componentTypeCount);
            for (USize i = 0; i < componentTypeCount; ++i)
            {
                types.Push(componentTypes[i].Convert());
            }

            *result = self->CreateEntity(types);
        }

        FE_DLL_EXPORT void EntityRegistry_CloneEntity(EntityRegistry* self, Entity* entity, Entity* result)
        {
            *result = self->CloneEntity(*entity);
        }

        FE_DLL_EXPORT void EntityRegistry_CreateEntities(EntityRegistry* self, Entity* entities, UInt32 entityCount)
        {
            self->CreateEntities(ArraySliceMut(entities, entities + entityCount));
        }

        FE_DLL_EXPORT void EntityRegistry_CreateEntitiesWithComponents(EntityRegistry* self, ComponentTypeBinding* componentTypes,
                                                                       UInt32 componentTypeCount, Entity* entities,
                                                                       UInt32 entityCount)
        {
            List<ComponentType> types;
            types.Reserve(componentTypeCount);
            for (USize i = 0; i < componentTypeCount; ++i)
            {
                types.Push(componentTypes[i].Convert());
            }

            self->CreateEntities(types, ArraySliceMut(entities, entities + entityCount));
        }

        FE_DLL_EXPORT void EntityRegistry_CloneEntityToArray(EntityRegistry* self, Entity* entity, Entity* entities,
                                                             UInt32 entityCount)
        {
            self->CloneEntity(*entity, ArraySliceMut(entities, entities + entityCount));
        }

        FE_DLL_EXPORT void EntityRegistry_DestroyEntity(EntityRegistry* self, Entity* entity)
        {
            self->DestroyEntity(*entity);
        }

        FE_DLL_EXPORT void EntityRegistry_DestroyEntities(EntityRegistry* self, Entity* entities, UInt32 entityCount)
        {
            self->DestroyEntities(ArraySlice(entities, entities + entityCount));
        }

        FE_DLL_EXPORT bool EntityRegistry_HasComponent(EntityRegistry* self, Entity* entity, ComponentTypeBinding* componentType)
        {
            return self->HasComponent(*entity, componentType->Convert());
        }

        FE_DLL_EXPORT bool EntityRegistry_CopyComponent(EntityRegistry* self, Entity* entity, ComponentTypeBinding* componentType,
                                                        void* destination)
        {
            return self->CopyComponent(*entity, componentType->Convert(), destination);
        }

        FE_DLL_EXPORT bool EntityRegistry_UpdateComponent(EntityRegistry* self, Entity* entity,
                                                          ComponentTypeBinding* componentType, const void* source)
        {
            return self->UpdateComponent(*entity, componentType->Convert(), source);
        }
    }
} // namespace FE::ECS
